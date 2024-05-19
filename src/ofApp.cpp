#include "macros.h"

#include "ofApp.h"

#include "colors.h"
#include "engine.h"
#include "game.h"
#include "global.h"
#include "generation.h"
#include "keyHandling.h"
#include "solver.h"
#include "testCases.h"
#include "visualsandide.h"

//--------------------------------------------------------------
void ofApp::setup(){
    //ofDisableSmoothing();
    cout << "You must run this code from the folder in which the binary is contained." << endl;
    ofDisableDataPath();
    
    initDefaultKeyMapping();
    
    initEditor(initialGame);

    ofSetEscapeQuitsApp(false);

    ofSetFrameRate(30); //don't do more work then necessary
    
    
    //keyHandling::keyQueue.push({KEY_IMPORT,0}); importing can take quite some time... maybe limit undo buffer?
    //executeKeys(); //emulate the import key press
    
    //initEditor(limeRick);
    //initEditor(sokobond);
    //initEditor(movementGarden);

    //testToCompileAllLevels();
    /*
    for(int i=0;i<editor::ideString.size();++i) {
        cerr << editor::ideString[i] << endl;
    }
    cerr << "==========================" << endl;*/

}




//--------------------------------------------------------------
void ofApp::update(){
    executeKeys();
}

//--------------------------------------------------------------

void ofApp::draw(){
    ofBackground(colors::colorBACKGROUND);
    
    if(gbl::mode == MODE_LEVEL_EDITOR || gbl::mode == MODE_EXPLOITATION || gbl::mode == MODE_INSPIRATION) {
        displayLevelEditor();
    } else if(gbl::mode == MODE_PLAYING) {
        displayPlayMode();
    }
    
    gbl::isMouseReleased = false;
    gbl::isFirstMousePressed = false;
#ifdef _WIN32
	static bool firstRun = true;
	if (firstRun) {
		HWND hWnd = ::GetActiveWindow();
		long l = GetWindowLong(hWnd, GWL_STYLE);
		SetWindowLong(hWnd, GWL_STYLE, l ^ WS_MINIMIZEBOX);
	}
	firstRun = false;
#endif
}

//--------------------------------------------------------------
static bool isSuperKey = false;
static bool isAltKey = false;
static bool isShiftKey = false;
void ofApp::keyPressed(int key){
    if(key == OF_KEY_SUPER || key == 3682) isSuperKey = true;
    if(key == OF_KEY_ALT || key == 3684) isAltKey = true;
    if(key == OF_KEY_SHIFT || key == OF_KEY_LEFT_SHIFT || key == OF_KEY_RIGHT_SHIFT || key == 3684) isShiftKey = true;

    if(editor::activeIDE) {
        ideKeyPressed(key, isSuperKey, isAltKey, isShiftKey);
    }
    else {
        keyHandle(key); // in keyHandler.h
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
    if(key == OF_KEY_SUPER || key == 3682 ) isSuperKey = false;
    if(key == OF_KEY_ALT || key == 3684) isAltKey = false;
    if(key == OF_KEY_SHIFT || key == OF_KEY_LEFT_SHIFT || key == OF_KEY_RIGHT_SHIFT || key == 3684) isShiftKey = false;

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    if(!gbl::isMousePressed) gbl::isFirstMousePressed = true;
    gbl::isMousePressed = true;
    if(gbl::mode == MODE_LEVEL_EDITOR || gbl::mode == MODE_EXPLOITATION || gbl::mode == MODE_INSPIRATION)  {
        if(!editor::activeIDE && x > ofGetWidth()*(1.-editor::ideFactor))
            switchToRightEditor(gbl::mode);
        else if( editor::activeIDE && x < ofGetWidth()*(1.-editor::ideFactor))
            switchToLeftEditor(gbl::mode, "switching_from_right_to_left");
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    gbl::isFirstMousePressed = false;
    gbl::isMousePressed = false;
    gbl::isMouseReleased = true;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
    
}

void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY) {
    if (scrollY > 0 && scrollY < 18) scrollY = 18;
    if (scrollY < 0 && scrollY > -18) scrollY = -18;
    
    editor::offsetIDEY += scrollY*4;
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 
    
}

void ofApp::exit() {
    cerr << "EXITING" << endl;
    keyHandling::keyQueue.push({KEY_PRINT,0});
    executeKeys();
    //stop solver 1
    stopSolving(0);
    //stop solver 2
    stopSolving(1);
    //stop generating
    stopGenerating();
}

// 10:30
