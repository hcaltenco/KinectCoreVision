#include "ofMain.h"
#include "testApp.h"
//#include "ofAppGlutWindow.h"

//========================================================================
int main(int argc, char *argv[])
{
    //ofAppGlutWindow window;
	ofSetupOpenGL(900, 900, OF_WINDOW); // HC: removed GLUT as recommended:
										// https://github.com/patriciogonzalezvivo/ofxFX/issues/44
										// otherwise get linker error

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new testApp(argc,argv));
}
