#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

// OpenNI headers
#include <XnOpenNI.h>
#include <XnTypes.h>
// NITE headers
#include <XnVSessionManager.h>
#include "XnVMultiProcessFlowClient.h"
#include <XnVWaveDetector.h>

#define SAMPLE_XML_FILE "/Users/holz/apps/Nite-1.3.0.18/Data/Sample-Tracking.xml"
#define WIDTH 1000
#define HEIGHT 1000

using namespace ci;
using namespace ci::app;
using namespace std;

Vec3f handCoords;


//-----------------------------------------------------------------------------
// Callbacks
//-----------------------------------------------------------------------------

// Callback for when the focus is in progress
void XN_CALLBACK_TYPE SessionProgress(const XnChar* strFocus, const XnPoint3D& ptFocusPoint, XnFloat fProgress, void* UserCxt)
{
	printf("Session progress (%6.2f,%6.2f,%6.2f) - %6.2f [%s]\n", ptFocusPoint.X, ptFocusPoint.Y, ptFocusPoint.Z, fProgress,  strFocus);
}
// callback for session start
void XN_CALLBACK_TYPE SessionStart(const XnPoint3D& ptFocusPoint, void* UserCxt)
{
	printf("Session started. Please wave (%6.2f,%6.2f,%6.2f)...\n", ptFocusPoint.X, ptFocusPoint.Y, ptFocusPoint.Z);
}
// Callback for session end
void XN_CALLBACK_TYPE SessionEnd(void* UserCxt)
{
	printf("Session ended. Please perform focus gesture to start session\n");
}
// Callback for wave detection
void XN_CALLBACK_TYPE OnWaveCB(void* cxt)
{
	printf("Wave!\n");
}
// callback for a new position of any hand
void XN_CALLBACK_TYPE OnPointUpdate(const XnVHandPointContext* pContext, void* cxt)
{
	handCoords = Vec3f(pContext->ptPosition.X + WIDTH/2, -pContext->ptPosition.Y + HEIGHT/2, -pContext->ptPosition.Z);
	printf("%d: (%f,%f,%f) [%f]\n", pContext->nID, pContext->ptPosition.X, pContext->ptPosition.Y, pContext->ptPosition.Z, pContext->fTime);
}

class OniTrackApp : public AppBasic {
  public:
	void prepareSettings(Settings* settings);
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	void shutdown();
	
	
	xn::Context context;
	XnVSessionGenerator* pSessionGenerator;
	XnBool bRemoting;
	XnVWaveDetector wc;
	
};

void OniTrackApp::prepareSettings(Settings* settings)
{
	settings->setWindowSize(WIDTH, HEIGHT);
}

void OniTrackApp::setup()
{
	bRemoting = FALSE;
	
	XnStatus rc = context.InitFromXmlFile(SAMPLE_XML_FILE);
	if (rc != XN_STATUS_OK)
	{
		printf("Couldn't initialize: %s\n", xnGetStatusString(rc));
		shutdown();
	}
	
	pSessionGenerator = new XnVSessionManager();
	rc = ((XnVSessionManager*)pSessionGenerator)->Initialize(&context, "Click", "RaiseHand");
	if (rc != XN_STATUS_OK)
	{
		printf("Session Manager couldn't initialize: %s\n", xnGetStatusString(rc));
		delete pSessionGenerator;
		shutdown();
	}
	
	
	context.StartGeneratingAll();
	
	pSessionGenerator->RegisterSession(NULL, &SessionStart, &SessionEnd, &SessionProgress);
	
	
	wc.RegisterWave(NULL, OnWaveCB);
	wc.RegisterPointUpdate(NULL, OnPointUpdate);
	pSessionGenerator->AddListener(&wc);
	
	printf("Please perform focus gesture to start session\n");
	printf("Hit any key to exit\n");
	
}

void OniTrackApp::mouseDown( MouseEvent event )
{
}

void OniTrackApp::update()
{
	context.WaitAndUpdateAll();
	((XnVSessionManager*)pSessionGenerator)->Update(&context);
}

void OniTrackApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
	
	gl::color(Color(1.0f, .0f, .0f));
	//gl::translate(handCoords);
	gl::drawSphere(handCoords, 50.0f, 64);
}

void OniTrackApp::shutdown()
{
	delete pSessionGenerator;
	
	context.Shutdown();
}


CINDER_APP_BASIC( OniTrackApp, RendererGl )
