#include <stdio.h>
#include "webService.h"
#include "soapH.h"
#include "dollySOAP.nsmap"
#include <sys/socket.h>
#include "camera.h"
#include <unistd.h>
#include <sys/types.h>
#include <iostream>

static struct soap *soapHandle;

SOAP_FMAC5 int SOAP_FMAC6 
__ns1__sendCmd(struct soap* soap, struct _ns1__sendCmd *ns1__sendCmd, struct _ns1__sendCmdResponse *ns1__sendCmdResponse)
{
	CWebService::WEBSERVICE_REQUEST *request = (CWebService::WEBSERVICE_REQUEST*) (soap->user);
	if (request->pCamera)
	{
    printf("Command %02x %02x \n", ns1__sendCmd->cmd, ns1__sendCmd->p1);
    
		switch (ns1__sendCmd->cmd)
		{
			case COMMAND_SET_ISO:
			{
				int ret = request->pCamera->SetIso(ns1__sendCmd->p1);
				break;
			}
			case COMMAND_SET_WB:
			{
				int ret = request->pCamera->SetWb(ns1__sendCmd->p1);
				break;
			}
			case COMMAND_SET_SHUTTER:
			{
				int ret = request->pCamera->SetSh(ns1__sendCmd->p1);
				break;
			}
			case COMMAND_SET_APERTURE:
			{
				int ret = request->pCamera->SetAp(ns1__sendCmd->p1);
				break;
			}
			case COMMAND_SET_EXPCOMP:
			{
				int ret = request->pCamera->SetEx(ns1__sendCmd->p1);
				break;
			}			
      case COMMAND_SET_IMGFORMAT:
			{
				int ret = request->pCamera->SetIf(ns1__sendCmd->p1);
				break;
			}	
      case COMMAND_SET_FOCUS:
			{ 				
				int ret = request->pCamera->SetManualFocus(ns1__sendCmd->p1);
				break;
			}			
			case COMMAND_IMG_CAPTURE:
			{
				int ret = request->pCamera->CaptureImage();
				break;
			}	
			case COMMAND_SERVO_HORIZONTAL:
			{
				int ret = request->pRobo->ServoHorizontal(ns1__sendCmd->p1);
				break;
			}
			case COMMAND_SERVO_VERTICAL:
			{
				int ret = request->pRobo->ServoVertical(ns1__sendCmd->p1);
				break;
			}
			case COMMAND_MOTOR_1:
			{
				int ret = request->pRobo->Motor1(ns1__sendCmd->p1);
				break;
			}
			case COMMAND_MOTOR_2:
			{
				int ret = request->pRobo->Motor2(ns1__sendCmd->p1);
				break;
			}
			case COMMAND_MOTOR_3:
			{
				int ret = request->pRobo->Motor3(ns1__sendCmd->p1);
				break;
			}
			case COMMAND_MOTOR_4:
			{
				int ret = request->pRobo->Motor4(ns1__sendCmd->p1);
				break;
			}
			default:
			{
			  break;
      }			 
		}
	/* int cmd int p1 int p2  */
	}
	return SOAP_OK;
}

SOAP_FMAC5 int SOAP_FMAC6 
__ns1__getConfig(struct soap* soap, struct _ns1__getConfig *ns1__getConfig, struct _ns1__getConfigResponse *ns1__getConfigResponse)
{
	int size = 0;
	CWebService::WEBSERVICE_REQUEST *request = NULL;
	request = (CWebService::WEBSERVICE_REQUEST*)((soap)->user);
	if (!request->pCamera)
	{
		printf("get config camera == NULL\n");
	}
	/* check for state invalidation flag */
	request->pCamera->config.GetXmlConfig((unsigned char**)&ns1__getConfigResponse->out, &size);
	return SOAP_OK;
}

CWebService::CWebService(CCamera *camera)
{
	pWebServiceThread = NULL;
	pEvents = NULL;
	pCamera = camera;
	pRobo = new CRobo;
//	soapMasterSocket = NULL;
}

CWebService::~CWebService(void)
{
	if (pWebServiceThread)
	{
		delete pWebServiceThread;
	}
	
	if (pEvents)
	{
		delete pEvents;
	}
	if (pRobo)
	{
		delete pRobo;
	}
}

void CWebService::Run(void)
{
	boost::bind(&CWebService::RunProc, pWebServiceThread);
}

void CWebService::Quit(void)
{
	pEvents->quit = true;
	/* kill soap master socket */
	printf("calling soap done\n");

	close(soapHandle->master);
	soapHandle->master = SOAP_INVALID_SOCKET;
	close(soapHandle->socket);
	soapHandle->socket = SOAP_INVALID_SOCKET;
	soap_destroy(soapHandle);
	soap_done(soapHandle);
  soap_end(soapHandle); 

	soapHandle = NULL;
	pEvents->condition.notify_one();
	
	pthread_cancel(pWebServiceThread->native_handle());
}

bool CWebService::Initialize(void)
{
	pEvents = new CWebService::WEBSERVICE_EVENT();
	if (!pEvents)
	{
		return false;
	}
	
	soapHandle = soap_new();
	if (!soap_init(soapHandle))
	{
		printf("Soap server initialization failed!\n");
		delete pEvents;
		return false;
	}
	
	//set_timeout(soapHandle);
	soapHandle->bind_flags = SO_REUSEADDR;
	//soapHandle->send_timeout = 200; // 60 seconds
  //soapHandle->recv_timeout = 200; // 60 seconds
  //soapHandle->max_keep_alive = 10; // max keep-alive sequence 
	
	if (soapMasterSocket = soap_bind(soapHandle, NULL, 8081, 100) < 0)
	{
		printf("bind failed\n");
		return false;
	}
	
	/*
	linger lin;
	lin.l_onoff = 1;
   lin.l_linger = 4; //seconds
   setsockopt(soapMasterSocket, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin));//(soapMasterSocket, SOL_SOCKET, SO_LINGER, reinterpret_cast(&lin), sizeof(lin));
	*/
	
	
	pWebServiceThread = new boost::thread(&CWebService::RunProc, this);
	
	if (!pWebServiceThread)
	{
		return false;
	}
	
	return true;
}

void CWebService::RunProc(void)
{
	SOAP_SOCKET s;
	boost::unique_lock<boost::mutex> lock(pEvents->mutex);
	for(;;)
	{
		pEvents->condition.timed_wait(lock, boost::posix_time::milliseconds(0));
		if (pEvents->quit)
		{
			printf("received quit event webservice thread\n");
			break;
		}
		/* timeout */
		else
		{
			WEBSERVICE_REQUEST *request = NULL;
			printf("wait for soap accept\n");
			s = soap_accept(soapHandle);
			if (!soap_valid_socket(soapMasterSocket))
			{
				break;
			}
			
			/* create soap request */
			request = new WEBSERVICE_REQUEST;
			if (!request)
			{
				break;
			}
			
			request->soap = soap_copy(soapHandle);
			if (!request->soap)
			{
				break;
			}
			
			/* create soap request thread */
			request->thisThread = new boost::thread(&CWebService::ProcessRequest, this, request);
			boost::bind(&CWebService::ProcessRequest, pWebServiceThread, request);
			printf("soap accept\n");
		}
	}
}

void CWebService::ProcessRequest(WEBSERVICE_REQUEST *request)
{
	printf(" process request\n ");
	request->pCamera = pCamera;
	request->pRobo = pRobo;
	request->soap->user = request;
	soap_serve(request->soap);
	soap_destroy(request->soap);
	soap_end(request->soap);
	soap_done(request->soap);
	free(request->soap);
  usleep(10000);
	request->thisThread->detach();
}
