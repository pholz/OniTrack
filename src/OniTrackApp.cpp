#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

// OpenNI headers
#include <XnOpenNI.h>
#include <XnTypes.h>
// NITE headers
#include <XnVSessionManager.h>
#include "XnVMultiProcessFlowClient.h"
#include <XnVWaveDetector.h>

#include "XnVNite.h"
#include "PointDrawer.h"

#define SAMPLE_XML_FILE "/Users/holz/apps/Nite-1.3.0.18/Data/Sample-Tracking.xml"
#define WIDTH 1000
#define HEIGHT 1000

using namespace ci;
using namespace ci::app;
using namespace std;

Vec3f handCoords;

#define CHECK_RC(rc, what)											\
if (rc != XN_STATUS_OK)											\
{																\
printf("%s failed: %s\n", what, xnGetStatusString(rc));		\
shutdown();													\
}

#define CHECK_ERRORS(rc, errors, what)		\
if (rc == XN_STATUS_NO_NODE_PRESENT)	\
{										\
XnChar strError[1024];				\
errors.ToString(strError, 1024);	\
printf("%s\n", strError);			\
shutdown();						\
}

SessionState g_SessionState = NOT_IN_SESSION;
// Draw the depth map?
XnBool g_bDrawDepthMap = true;
XnBool g_bPrintFrameID = false;
// Use smoothing?
XnFloat g_fSmoothing = 0.0f;
XnBool g_bPause = false;
XnBool g_bQuit = false;

//-----------------------------------------------------------------------------
// Callbacks
//-----------------------------------------------------------------------------

// Callback for when the focus is in progress
void XN_CALLBACK_TYPE FocusProgress(const XnChar* strFocus, const XnPoint3D& ptPosition, XnFloat fProgress, void* UserCxt)
{
	//	printf("Focus progress: %s @(%f,%f,%f): %f\n", strFocus, ptPosition.X, ptPosition.Y, ptPosition.Z, fProgress);
}
// callback for session start
void XN_CALLBACK_TYPE SessionStarting(const XnPoint3D& ptPosition, void* UserCxt)
{
	printf("Session start: (%f,%f,%f)\n", ptPosition.X, ptPosition.Y, ptPosition.Z);
	g_SessionState = IN_SESSION;
}
// Callback for session end
void XN_CALLBACK_TYPE SessionEnding(void* UserCxt)
{
	printf("Session end\n");
	g_SessionState = NOT_IN_SESSION;
}
void XN_CALLBACK_TYPE NoHands(void* UserCxt)
{
	if (g_SessionState != NOT_IN_SESSION)
	{
		printf("Quick refocus\n");
		g_SessionState = QUICK_REFOCUS;
	}
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
	xn::DepthGenerator depthGenerator;
	xn::HandsGenerator handsGenerator;
	XnVSessionGenerator* pSessionGenerator;
	XnBool bRemoting;
	XnVWaveDetector wc;
	XnVFlowRouter* pFlowRouter;
	
	// the drawer
	XnVPointDrawer* pDrawer;
	
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
	
	rc = context.FindExistingNode(XN_NODE_TYPE_DEPTH, depthGenerator);
	CHECK_RC(rc, "Find depth generator");
	rc = context.FindExistingNode(XN_NODE_TYPE_HANDS, handsGenerator);
	CHECK_RC(rc, "Find hands generator");
	
	pSessionGenerator = new XnVSessionManager();
	rc = ((XnVSessionManager*)pSessionGenerator)->Initialize(&context, "Click,Wave", "RaiseHand");
	if (rc != XN_STATUS_OK)
	{
		printf("Session Manager couldn't initialize: %s\n", xnGetStatusString(rc));
		delete pSessionGenerator;
		shutdown();
	}
	
	
	//context.StartGeneratingAll();
	
	pSessionGenerator->RegisterSession(NULL, SessionStarting, SessionEnding, FocusProgress);
	pDrawer = new XnVPointDrawer(20, depthGenerator); 
	pFlowRouter = new XnVFlowRouter;
	pFlowRouter->SetActive(pDrawer);
	
	pSessionGenerator->AddListener(pFlowRouter);
	
	pDrawer->RegisterNoPoints(NULL, NoHands);
	pDrawer->SetDepthMap(g_bDrawDepthMap);
	
	context.StartGeneratingAll();
	
}

void OniTrackApp::mouseDown( MouseEvent event )
{
}

void OniTrackApp::update()
{
	
}

void OniTrackApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
	
	//gl::color(Color(1.0f, .0f, .0f));
	//gl::translate(handCoords);
	//gl::drawSphere(handCoords, 50.0f, 64);
	
	context.WaitAndUpdateAll();
	((XnVSessionManager*)pSessionGenerator)->Update(&context);
	
	//Vec3f v = pDrawer->getLatestPoint();
	//console() << v.x << "//" << v.y << "//" << v.z << endl;
	list<Vec3f>* pts = pDrawer->getLatestPoints();
	
	list<Vec3f>::const_iterator it;
	for(it = pts->begin(); it != pts->end(); it++)
	{
		gl::color(Color(1.0f, .0f, .0f));
		gl::drawSphere(*it * Vec3f(1.0f, 1.0f, -.25f), 50.0f, 64);
	}
}

void OniTrackApp::shutdown()
{
	delete pSessionGenerator;
	
	context.Shutdown();
}


CINDER_APP_BASIC( OniTrackApp, RendererGl )
