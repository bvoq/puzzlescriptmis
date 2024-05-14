#include "ofMain.h"
#include "ofApp.h"

#include "generation.h"
#include "solver.h"


// this combined with: https://stackoverflow.com/questions/2139637/hide-console-of-windows-application

#ifdef WIN32
//windows subsystem
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	cout << "windows" << endl;
	//ofAppGlutWindow window;
//window.setGlutDisplayString("rgba double samples>=4");
	ofSetupOpenGL(1024, 768, OF_WINDOW);			// <-------- setup the GL context

	//SetWindowLong(OF_WINDOW, GWL_STYLE, )
	
	
	//Hide window:
	//ShowWindow (GetConsoleWindow(), SW_HIDE); //should be hidden by default...

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());
}

/*
BOOL WINAPI ConsoleHandler(DWORD CEvent)
{
	if (CEvent == CTRL_CLOSE_EVENT)
	{
		stopGenerating();
		stopSolving(0);
		stopSolving(1);
	}
	return TRUE;
}
*/
#else
//========================================================================
int main( ){
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}
#endif

// games:

/*
 https://www.puzzlescript.net/play.html?p=ff7bedeb7248abd94781d6dc9d1e63ca
 TWO SNAKEOBAN CLONES:
 http://www.puzzlescript.net/play.html?p=7019377
 https://www.puzzlescript.net/editor.html?hack=e5561ef165d87310166e
 */



/*
 
 TODO:
 DONE Create a new legend if object is not present => When choosing it.
 DONE Display passive information about the level (solver info).
 DONE Generate button
 - IDE syntax highlight
 DONE add own solution
 DONE stationary move block
 DONE Add stationary move to solver
 - Don't do dppair with copy of game perhaps?
 - maybe use dynamic compilation of PS files into a TinyCC compiler 
 */
