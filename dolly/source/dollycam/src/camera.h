#pragma once
#include <gphoto2/gphoto2-camera.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/impl/io_service.hpp>
#include <gphoto2/gphoto2-camera.h>

#include "webService.h"
#include "cameraConfig.h"

class CWebService;

class CCamera{
	
public:
	/* constructor */
	CCamera(void);
	/* destructor */
	~CCamera(void);
	bool Initialize();
	void Run();
	void Quit();
	CCamera *GetCamera();
	
	
	/* camera access function */
	bool LiveViewStart();
	bool LiveViewStreamImage();
	bool LiveViewStop();
	int SetIso(int position);
  int SetWb(int position);
  int SetSh(int position);
  int SetAp(int position);
  int SetEx(int position);
  int SetIf(int position);
  int SetManualFocus(int position);
  int CaptureImage(void);
  void CaptureThread(void);
private:
	/* events structure */
	typedef struct _CAMERA_EVENT
	{
		bool quit;
		boost::condition_variable condition;
		boost::mutex eventMutex;
	}CAMERA_EVENT, *PCAMERA_EVENT;
	
	typedef enum _CAMERA_STATE
	{
		CAMERA_NOT_PRESENT = 1,
		CAMERA_NOT_CONNECTED,
		CAMERA_INITIALIZED
	}CAMERA_STATE;
	
	boost::thread *pCameraThread;
	boost::thread *pCaptThread;
	CWebService *pWebService;
	void RunProc(void);
	CAMERA_MODE GetCameraMode();
	bool GetFocusMode();
	void UpdateIso();
  void UpdateWb();
  void UpdateSh();
  void UpdateAp();
  void UpdateEx();
  void UpdateIf();
  void UpdateCi();
  void UpdateBt();
	bool PrintWidget();
	int CaptureTargetOn();
	/* events container */
	CAMERA_EVENT *pEvents;
	Camera *pCamera;
	GPContext *pContext;
	GPPortInfoList		*portinfolist;
	CameraAbilitiesList	*abilities;
	CAMERA_STATE cameraState;
	int streamPipe;
	CAMERA_MODE cameraMode;
	bool afMode;
	boost::mutex cameraMutex;
	bool captInProgress;
public:
	CCameraConfig config;
};