#include "visualsandide.h"

#include "colors.h"
#include "game.h"
#include "generation.h"
#include "global.h"
#include "levels.h"
#include "logError.h"
#include "keyHandling.h"
#include "objects.h"
#include "recordandundo.h"
#include "rules.h"
#include "solver.h"
#include "testCases.h"

#ifndef compiledWithoutOpenframeworks
#include "ofApp.h"

namespace editor {
    
    bool activeIDE = false;
    bool generatorIDE = false;
    int showGenerate = 0; //0=no show, 1=show, 2=show+stats
    float ideFactor = 3./7; //how much of the screen should be IDE
    float offsetIDEY = 0;
    int levelIndex = 0;
    int selectedExploitationTool = -1;
    int selectedBlock = -1; //if selectedBlock < gbl::currentGame.objPrimaryName.size() => selectedBlock = id | else (selectedBlock - gbl::currentGame.objPrimaryName.size() ) = gbl::currentGame.aggsWithSingleCharName id
    MODE_TYPE previousMenuMode = MODE_LEVEL_EDITOR;
    string selectedBlockStr = ""; //single character symbol
    
    vector<string> ideString ; // either from levelEditor or from exploitation
    
    vector<vector<vector<bool> > > modifyTable; //the modify/leave map

    vector<string> levelEditorString;
    vector<string> exploitationString;
    
    pair<bool,bool> successes; //first = levelEditor succesfully compiled, second = generator succesfully compiled
    
    static vector<string> emptyExploitationString = {
        "==========",
        "TRANSFORM",
        "==========",
        "[] -> []"};
    
    static pair<int,int> cursorPos = {0,0}, selectPos = {-1,-1};
    static float ideWidth = -1, prevIdeWidth = -2;
    static int ideLongestSentence = -1, prevIdeLongestSentence = -2;
    
    static int ideLastErrorLine = -1, ideLastWarningLine = -1;
    static string ideLastErrorStr = "", ideLastWarningStr = "";
    
    static ofTrueTypeFont ideFont; //License see 1.4 http://theleagueof.github.io/licenses/ofl-faq.html
    static ofTrueTypeFont blockSelectionFont;
    static ofTrueTypeFont buttonFont;
    static ofTrueTypeFont singleCharFont;
    
    static float prevScaleBlockSelection = -1;
    
}
using namespace editor;

//store / load the IDE depending on the mode
static void refreshIDEStrings(MODE_TYPE oldmode, MODE_TYPE newmode) {
    if(oldmode == MODE_LEVEL_EDITOR || oldmode == MODE_INSPIRATION)
        levelEditorString = ideString;
    else if(oldmode == MODE_EXPLOITATION)
        exploitationString = ideString;
    
    if(newmode == MODE_LEVEL_EDITOR || newmode == MODE_INSPIRATION)
        ideString = levelEditorString;
    else if(newmode == MODE_EXPLOITATION)
        ideString = exploitationString;
    
    gbl::mode = newmode;
    editor::previousMenuMode = newmode;
}

void initEditor(vector<string> ingame) {
    activeIDE = false;
    cursorPos = {0,0};
    selectPos = {-1,-1};
    selectedBlock = -1;
    selectedExploitationTool = -1;
    offsetIDEY = 0;
    ideString = ingame;
    
    //assumes this works!
    levelEditorString = ingame;
    exploitationString = {
        "==========",
        "TRANSFORM",
        "==========",
        "[] -> []",
    };
    
    successes = parseGame(levelEditorString, exploitationString, gbl::currentGame, logger::levelEdit, logger::generator);
    synchronized(generator::generatorMutex) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        generator::generatorNeighborhood.resize( gbl::currentGame.levels.size());
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    assert(successes.first && successes.second); //should always work, since it's the default initial game
    switchToLevel(0, gbl::currentGame);
    switchToLeftEditor(MODE_LEVEL_EDITOR, "init_push");
    
    //if(!success) switchToIDE();
    //else switchAwayFromIDE();
    
    //texModif = ;
    //texLeave = ;
}

void switchToLeftEditor(MODE_TYPE newmode, const string reason) {
    assert(newmode == MODE_LEVEL_EDITOR || newmode == MODE_EXPLOITATION || newmode == MODE_INSPIRATION);
    refreshIDEStrings(gbl::mode, newmode); //rewrite leveleditorstring
    
    bool restartGenerating = false;
    static Game oldGame = gbl::currentGame;
    //detects any changes to the game / editor or if there have been errors in the previous IDE build.
    if(gbl::record.editorHistory.size() == 0
       || ideLastErrorStr != ""
       || gbl::currentGame.currentLevelIndex != get<1>(gbl::record.editorHistory.back().second).currentLevelIndex
       || levelEditorString != get<3>(gbl::record.editorHistory.back().second)
       || exploitationString != get<4>(gbl::record.editorHistory.back().second)
       || modifyTable != get<5>(gbl::record.editorHistory.back().second)) {
        stopGenerating();
        
        //RECOMPILE CURRENT GAME
        oldGame = gbl::currentGame;
        successes = parseGame(levelEditorString, exploitationString, gbl::currentGame, logger::levelEdit, logger::generator);
        if(successes.first && !successes.second && newmode != MODE_EXPLOITATION) {
            exploitationString = emptyExploitationString;
            successes = parseGame(levelEditorString, exploitationString, gbl::currentGame, logger::levelEdit, logger::generator);
        }
        if(successes.first && successes.second) restartGenerating = true;
    }
    //READJUST ERROR MSG FROM COMPILING AND CURSOR BASED ON ERROR
    ideLastErrorLine = gbl::mode == MODE_LEVEL_EDITOR ? logger::levelEdit.lastErrorLine : logger::generator.lastErrorLine;
    ideLastErrorStr = gbl::mode == MODE_LEVEL_EDITOR ? logger::levelEdit.lastErrorStr : logger::generator.lastErrorStr;
    ideLastWarningLine = gbl::mode == MODE_LEVEL_EDITOR ? logger::levelEdit.lastWarningLine : logger::generator.lastWarningLine;
    ideLastWarningStr = gbl::mode == MODE_LEVEL_EDITOR ? logger::levelEdit.lastWarningStr : logger::generator.lastWarningStr;
    if(ideLastErrorLine >= 0 && ideLastErrorLine < ideString.size()) {
        cursorPos.first = ideLastErrorLine;
        cursorPos.second = 0;
        selectPos = cursorPos;
        readjustIDEOffset();
    }
    
    if((gbl::mode == MODE_LEVEL_EDITOR && !successes.first) || (gbl::mode == MODE_EXPLOITATION && !successes.second)) {
        gbl::currentGame = oldGame;
        switchToRightEditor(newmode); //didn't compile, go back to old ide
        //generator::generatorMutex.lock();
        //generator::generatorMutex.unlock();
        //pushEditorState(gbl::record, reason);
        return;
    }
    synchronized(generator::generatorMutex) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        generator::generatorNeighborhood.resize( gbl::currentGame.levels.size());
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
        
    // READJUST CURRENT LEVEL INDEX
    switchToLevel(MIN(oldGame.currentLevelIndex, (int)gbl::currentGame.levels.size() - 1), gbl::currentGame);
    
    // READJUST MODIFY TABLE
    if(modifyTable.size() != gbl::currentGame.levels.size()) {
        modifyTable.resize(gbl::currentGame.levels.size(), {});
    }
    for(int i=0; i<gbl::currentGame.levels.size(); ++i) {
        if(modifyTable[i].size() != gbl::currentGame.levels[i][0].size()
           || modifyTable[i][0].size() != gbl::currentGame.levels[i][0][0].size() ) {
            int layerH = gbl::currentGame.levels[i][0].size(), layerW = gbl::currentGame.levels[i][0][0].size();
            vector<vector<bool> > mTable(layerH, vector<bool> (layerW, true));
            modifyTable[i] = mTable;
        }
    }
    
    cout << "modifytablesize: " << modifyTable.size() << " " << modifyTable[0].size() << endl;
    
    //make sure activeIDE = false
    activeIDE = false;
    
    pushEditorState(gbl::record, reason);
    if(restartGenerating) startGenerating();
}

void switchToRightEditor(MODE_TYPE newmode) { //switch to IDE
    assert(newmode == MODE_LEVEL_EDITOR || newmode == MODE_EXPLOITATION || newmode == MODE_INSPIRATION);
    
    activeIDE = true;
    selectedBlock = -1;
    selectedExploitationTool = -1;
    
    if(!(cursorPos.first < ideString.size() && cursorPos.second <= ideString[cursorPos.first].size() )) {
        cursorPos = {0,0};
    }
    selectPos = cursorPos;
}


void readjustIDEOffset() {
    int lineHeight = ideFont.getLineHeight();
    int roughOffsetIDE = -(cursorPos.first * lineHeight);
    
    //if(cursorPos.first*lineHeight + offsetIDE < 0 || cursorPos.first*lineHeight + offsetIDE > ofGetHeight())
    offsetIDEY = -cursorPos.first*lineHeight + ofGetHeight()/2;
    
    offsetIDEY = MIN(0,offsetIDEY);
    offsetIDEY = MAX(-1 *((int)ideString.size()) * lineHeight, offsetIDEY);
}

void displayLevelEditor() {
    if(gbl::mode == MODE_LEVEL_EDITOR)
        colors::colorIDE_BACKGROUND = ofColor(0x11,0x18,0x28);
    if(gbl::mode == MODE_EXPLOITATION)
        colors::colorIDE_BACKGROUND = ofColor(0x28,0x18,0x11);
    if(gbl::mode == MODE_INSPIRATION)
        colors::colorIDE_BACKGROUND = ofColor(0x11,0x28,0x18);
    
    ideWidth = ofGetWidth() * ideFactor;
    ideLongestSentence = 72;
    for(const auto & t : ideString) ideLongestSentence = MAX(ideLongestSentence, t.size() + 15);
    if(ideLastErrorStr.size() > 0) {
        ideLongestSentence = MAX(ideLongestSentence, MIN(150,ideLastErrorStr.size()+16));
        if(ideLastErrorLine >= 0 && ideLastErrorLine < ideString.size()) {
            ideLongestSentence = MAX(ideLongestSentence, MIN(150, ideLastErrorStr.size()+ideString[ideLastErrorLine].size()+16));
        }
    }
    if(ideLastWarningStr.size() > 0) {
        ideLongestSentence = MAX(ideLongestSentence, MIN(150,ideLastWarningStr.size()+16));
        if(ideLastWarningLine >= 0 && ideLastWarningLine < ideString.size()) {
            ideLongestSentence = MAX(ideLongestSentence, MIN(150, ideLastWarningStr.size()+ideString[ideLastWarningLine].size()+16));
        }
    }
    
    
    int objCount = MAX(5,gbl::currentGame.synsWithSingleCharName.size()+gbl::currentGame.aggsWithSingleCharName.size());
    float scaleBlockSelection = ofGetWidth() * (1.-ideFactor) / objCount;
    
    //compute the scale required for the largest block editor.
    int largestFontWidth = 6;
    for(int i=0; i<gbl::currentGame.synsWithSingleCharName.size(); ++i)
        largestFontWidth = MAX( largestFontWidth, gbl::currentGame.objPrimaryName[gbl::currentGame.synsWithSingleCharName[i].second].size()+2 );
    
    
    float scaleBlockSelectionFont = scaleBlockSelection / largestFontWidth;
    
    if(prevIdeWidth != ideWidth || prevIdeLongestSentence != ideLongestSentence || scaleBlockSelectionFont != prevScaleBlockSelection) {
        //load a font relative to the width of the display editor (might change later)
        cout << "Loading ideFont: " << gbl::locationOfResources + "font/Inconsolata/Inconsolata-Regular.ttf" << " of size " << ideWidth * 1./46*1./72 * ideLongestSentence << endl;
        ideFont.load(gbl::locationOfResources + "font/Inconsolata/Inconsolata-Regular.ttf",ideWidth * 1./46 * 72/ideLongestSentence);
        cout << "Loading blockSelectionFont: " << gbl::locationOfResources + "font/Inconsolata/Inconsolata-Regular.ttf" << " of size " << scaleBlockSelectionFont*1.5 << endl;
        blockSelectionFont.load(gbl::locationOfResources + "font/Inconsolata/Inconsolata-Regular.ttf",scaleBlockSelectionFont*1.5);
        cout << "Loading buttonFont: " << gbl::locationOfResources + "font/Inconsolata/Inconsolata-Regular.ttf" << " of size " << MIN(ofGetWidth(),ofGetHeight())/75 << endl;
        buttonFont.load(gbl::locationOfResources + "font/Inconsolata/Inconsolata-Regular.ttf",MIN(ofGetWidth(),ofGetHeight())/85);
        singleCharFont.load(gbl::locationOfResources + "font/Inconsolata/Inconsolata-Regular.ttf", scaleBlockSelection*.5);
        
        prevIdeWidth = ideWidth;
        prevIdeLongestSentence = ideLongestSentence;
        prevScaleBlockSelection = scaleBlockSelectionFont;
        
        readjustIDEOffset(); //whenever font gets changed, so does the cursor
    }
    
    if(gbl::mode == MODE_LEVEL_EDITOR) {
        for(int i=0;i<gbl::currentGame.synsWithSingleCharName.size();++i) {
            int x=i*scaleBlockSelection, y=ofGetHeight()-scaleBlockSelection;
            
            short objId = gbl::currentGame.synsWithSingleCharName[i].second;
            
            if(ofGetAppPtr()->mouseX >= x && ofGetAppPtr()->mouseX < x + scaleBlockSelection && ofGetAppPtr()->mouseY > y-scaleBlockSelectionFont*2 && ofGetAppPtr()->mouseY <= y+scaleBlockSelection) {
                
                ofSetColor(0xff,0x55,0x55);
                ofFill();
                ofDrawRectangle(x, y-scaleBlockSelectionFont*2, scaleBlockSelection, scaleBlockSelection+scaleBlockSelectionFont*2);
                if(gbl::isMousePressed) {
                    selectedBlock = objId;
                    selectedBlockStr = gbl::currentGame.synsWithSingleCharName[i].first;
                }
            }
            
            if(selectedBlock == objId) {
                ofSetColor(0x55,0x55,0xff);
                ofFill();
                ofDrawRectangle(x, y-scaleBlockSelectionFont*2, scaleBlockSelection, scaleBlockSelection+scaleBlockSelectionFont*2);
            }
            
            ofNoFill();
            ofSetColor(0xff);
            
            colors::textures[ gbl::currentGame.objTexture[objId ] ].draw(x,y,scaleBlockSelection,scaleBlockSelection);
            ofSetColor(0x0);
            float offset = MAX(0, (scaleBlockSelection - blockSelectionFont.stringWidth(gbl::currentGame.objPrimaryName[objId] + " "+gbl::currentGame.synsWithSingleCharName[i].first) )/2. );
            blockSelectionFont.drawString(gbl::currentGame.objPrimaryName[ objId ] + " "+gbl::currentGame.synsWithSingleCharName[i].first, x + offset, y-scaleBlockSelectionFont*.5);
            //singleCharFont.drawString(gbl::currentGame.synsWithSingleCharName[i].second, x+scaleBlockSelection*.25,y+scaleBlockSelection*.75);
        }
        for(int i=0;i<gbl::currentGame.aggsWithSingleCharName.size();++i) {
            
            int x=(i+gbl::currentGame.synsWithSingleCharName.size())*scaleBlockSelection, y=ofGetHeight()-scaleBlockSelection;
            
            //short objId = gbl::currentGame.aggsWithSingleCharName[i].second;
            
            if(ofGetAppPtr()->mouseX >= x && ofGetAppPtr()->mouseX < x + scaleBlockSelection && ofGetAppPtr()->mouseY > y-scaleBlockSelectionFont*2 && ofGetAppPtr()->mouseY <= y+scaleBlockSelection) {
                ofSetColor(0xff,0x55,0x55);
                ofFill();
                ofDrawRectangle(x, y-scaleBlockSelectionFont*2, scaleBlockSelection, scaleBlockSelection+scaleBlockSelectionFont*2);
                if(gbl::isMousePressed) {
                    selectedBlock = i+gbl::currentGame.objLayer.size();
                    selectedBlockStr = gbl::currentGame.aggsWithSingleCharName[i].first;
                }
            }
            
            if(selectedBlock == i+gbl::currentGame.objLayer.size()) {
                ofSetColor(0x55,0x55,0xff);
                ofFill();
                ofDrawRectangle(x, y-scaleBlockSelectionFont*2, scaleBlockSelection, scaleBlockSelection+scaleBlockSelectionFont*2);
            }
            
            ofNoFill();
            ofSetColor(0xff);
            
            for(short objId : gbl::currentGame.aggsWithSingleCharName[i].second) {
                colors::textures[ gbl::currentGame.objTexture[ objId ] ].draw(x,y,scaleBlockSelection, scaleBlockSelection);
            }
            
            ofSetColor(0x0);
            float offset = MAX(0, (scaleBlockSelection - blockSelectionFont.stringWidth(gbl::currentGame.aggsWithSingleCharName[i].first) )/2. );
            blockSelectionFont.drawString(gbl::currentGame.aggsWithSingleCharName[i].first, x + offset, y-scaleBlockSelectionFont*.5);
            //singleCharFont.drawString(gbl::currentGame.synsWithSingleCharName[i].second, x+scaleBlockSelection*.25,y+scaleBlockSelection*.75);
        }
    } else if(gbl::mode == MODE_EXPLOITATION) {
        for(int i=0;i<2;++i) {
            int x=i*scaleBlockSelection, y=ofGetHeight()-scaleBlockSelection;
            
            if(ofGetAppPtr()->mouseX >= x && ofGetAppPtr()->mouseX < x + scaleBlockSelection && ofGetAppPtr()->mouseY > y-scaleBlockSelectionFont*2 && ofGetAppPtr()->mouseY <= y+scaleBlockSelection) {
                ofSetColor(0xff,0x55,0x55);
                ofFill();
                ofDrawRectangle(x, y-scaleBlockSelectionFont*2, scaleBlockSelection, scaleBlockSelection+scaleBlockSelectionFont*2);
                if(gbl::isMousePressed) {
                    selectedExploitationTool = i;
                }
            }
            
            if(selectedExploitationTool == i) {
                ofSetColor(0x55,0x55,0xff);
                ofFill();
                ofDrawRectangle(x, y-scaleBlockSelectionFont*2, scaleBlockSelection, scaleBlockSelection+scaleBlockSelectionFont*2);
            }
            
            ofNoFill();
            ofSetColor(0xff);
            ofSetLineWidth(scaleBlockSelection/10);
            if(i == 0) ofSetColor(0xff,0,0);
            else if(i == 1) ofSetColor(0,0xff,0);
            
            string cstr = "Leave";
            if(i==1) cstr = "Modify";
            ofDrawRectangle(x+scaleBlockSelection/20,y+scaleBlockSelection/20,scaleBlockSelection-scaleBlockSelection/10,scaleBlockSelection-scaleBlockSelection/10);
            //gbl::currentGame.objTexture[ objId ].draw(x,y,scaleBlockSelection,scaleBlockSelection);
            
            ofSetLineWidth(1);
            ofSetColor(0x0);
            float offset = MAX(0, (scaleBlockSelection - blockSelectionFont.stringWidth(cstr) )/2. );
            
            blockSelectionFont.drawString(cstr, x + offset, y-scaleBlockSelectionFont*.5);
        }
    }
    
    //DRAW IDE
    int lineHeight = ideFont.getLineHeight();
    int heightBetween0lineAndNextLine = (ideFont.stringHeight("q")- ideFont.stringHeight("a"))*2./3;
    
    offsetIDEY = MIN(0,offsetIDEY);
    offsetIDEY = MAX(-1 *((int)ideString.size()) * lineHeight, offsetIDEY);
    
    float offsetIDEX = ofGetWidth()-ideWidth+heightBetween0lineAndNextLine*2;
    
    ofFill();
    ofSetColor(colors::colorIDE_BACKGROUND);
    ofDrawRectangle(ofGetWidth()-ideWidth,0,ideWidth,ofGetHeight());
    for(int i = 0; i < ideString.size();++i) {
        int cY = ofGetHeight()/24+i*lineHeight + offsetIDEY;
        ofNoFill();
        ofSetColor(colors::colorIDE_TEXT);
        ideFont.drawString(ideString[i], offsetIDEX,cY);
        
        if(ideLastErrorLine == i) {
            ofFill();
            ofSetColor(0xff,0x00,0x00, 0x90);
            int sY = ofGetHeight()/24+i*lineHeight - lineHeight + heightBetween0lineAndNextLine + offsetIDEY;
            ofDrawRectangle(ofGetWidth()-ideWidth,sY+lineHeight-3,ideWidth,3);
            ofNoFill();
            ofSetColor(0xff,0,0);
            float strW = ideFont.stringWidth(ideString[i]+"WW");
            ideFont.drawString("Error: "+ideLastErrorStr, offsetIDEX+strW,cY);
        }
        else if(ideLastWarningLine == i) {
            ofFill();
            ofSetColor(0xff,0xff,0x00, 0x90);
            int sY = ofGetHeight()/24+i*lineHeight - lineHeight + heightBetween0lineAndNextLine + offsetIDEY;
            ofDrawRectangle(ofGetWidth()-ideWidth,sY+lineHeight-3,ideWidth,3);
            ofNoFill();
            ofSetColor(0xff,0xff,0);
            float strW = ideFont.stringWidth(ideString[i]+"WW");
            ideFont.drawString("Warning: "+ideLastWarningStr, offsetIDEX+strW,cY);
        }
    }
    
    if(activeIDE) { //SELECT CURSOR POS
        int mX = ofGetAppPtr()->mouseX, mY = ofGetAppPtr()->mouseY;
        
        if(gbl::isMousePressed) {
            for(int i=0;i<ideString.size();++i) {
                int sY = ofGetHeight()/24+i*lineHeight - lineHeight + heightBetween0lineAndNextLine + offsetIDEY;
                int eY = sY+lineHeight;
                int sX=0,eX=0;
                if(!(mY >= sY && mY < eY)) continue;
                
                for(int j=0;j<ideString[i].size();++j) {
                    string stringToGetWidth = ideString[i].substr(0,j);
                    for(int i=stringToGetWidth.size()-1;i>=0;i--) if(stringToGetWidth[i] == ' ') stringToGetWidth[i] = '_';
                    int widthOfStringUntilCursor = ideFont.stringWidth(stringToGetWidth);
                    int widthOfCharacterOnCursor = ideFont.stringWidth("W"); //getGlyphBBox().getWidth();
                    
                    sX = offsetIDEX + widthOfStringUntilCursor;
                    eX = sX + widthOfCharacterOnCursor;
                    
                    if(mX >= sX && mX < eX && mY >= sY && mY < eY) {
                        cursorPos.first = i;
                        cursorPos.second = j;
                        if(gbl::isFirstMousePressed) selectPos = cursorPos;
                    }
                }
                if(mX >= eX) {
                    cursorPos.first = i;
                    cursorPos.second = ideString[i].size();
                    if(gbl::isFirstMousePressed) selectPos = cursorPos;
                }
                if(mX <= offsetIDEX) {
                    cursorPos.first = i;
                    cursorPos.second = 0;
                    if(gbl::isFirstMousePressed) selectPos = cursorPos;
                }
            }
            if(mY < ofGetHeight()/24+0*lineHeight - lineHeight + heightBetween0lineAndNextLine + offsetIDEY) {
                cursorPos.first = 0;
                cursorPos.second = 0;
                if(gbl::isFirstMousePressed) selectPos = cursorPos;
            }
            if(mY >= ofGetHeight()/24+ideString.size()*lineHeight - lineHeight + heightBetween0lineAndNextLine + offsetIDEY) {
                cursorPos.first = ideString.size()-1;
                cursorPos.second = ideString[cursorPos.first].size();
                if(gbl::isFirstMousePressed) selectPos = cursorPos;
            }
        }
        
        if(cursorPos != selectPos) { //DISPLAY YELLOW MARKED TEXT
            auto minPos = MIN(cursorPos, selectPos);
            auto maxPos = MAX(cursorPos, selectPos);
            
            for(int i=minPos.first;i<=maxPos.first;++i) {
                int sY = ofGetHeight()/24+i*lineHeight - lineHeight + heightBetween0lineAndNextLine + offsetIDEY;
                int eY = sY+lineHeight;
                
                int fromX = offsetIDEX;
                int toX = ofGetWidth();
                
                if(minPos.first == i) {
                    string stringToGetWidth = ideString[minPos.first].substr(0,minPos.second);
                    for(int i=stringToGetWidth.size()-1;i>=0;i--) if(stringToGetWidth[i] == ' ') stringToGetWidth[i] = '_';
                    
                    int widthOfStringUntilCursor = ideFont.stringWidth(stringToGetWidth);
                    fromX = offsetIDEX + ideFont.stringWidth(stringToGetWidth);
                }
                
                if(maxPos.first == i) {
                    string stringToGetWidth = ideString[maxPos.first].substr(0,maxPos.second);
                    for(int i=stringToGetWidth.size()-1;i>=0;i--) if(stringToGetWidth[i] == ' ') stringToGetWidth[i] = '_';
                    
                    int widthOfStringUntilCursor = ideFont.stringWidth(stringToGetWidth);
                    toX = offsetIDEX+ideFont.stringWidth(stringToGetWidth);
                }
                
                ofFill();
                ofSetColor(0xff,0xff,0x00, 0x90);
                ofDrawRectangle(fromX,sY,toX-fromX,lineHeight);
            }
        }
    }
    
    
    if(activeIDE) { // DRAW IDE
        string stringToGetWidth = ideString[cursorPos.first].substr(0,cursorPos.second);
        for(int i=stringToGetWidth.size()-1;i>=0;i--) if(stringToGetWidth[i] == ' ') stringToGetWidth[i] = '_';
        
        int widthOfStringUntilCursor = ideFont.stringWidth(stringToGetWidth);
        int widthOfCharacterOnCursor = ideFont.stringWidth("W"); //getGlyphBBox().getWidth();
        
        ofFill();
        ofSetColor(colors::colorIDE_CURSOR, 0x90);
        
        if(ofGetElapsedTimeMicros() % 1000000 < 750000)
            ofDrawRectangle(offsetIDEX + widthOfStringUntilCursor, ofGetHeight()/24+cursorPos.first*lineHeight - lineHeight + heightBetween0lineAndNextLine + offsetIDEY, widthOfCharacterOnCursor, lineHeight);
    }
    
    
    ofFill();
    ofSetColor(colors::colorIDE_BACKGROUND);
    int widthPlusButton = buttonFont.stringWidth("_+_");
    int widthArrowButton = buttonFont.stringWidth("_<-_");
    int widthPlayButton = buttonFont.stringWidth("_Play (R)_");
    int widthUndoButton = buttonFont.stringWidth("_Undo (U)_");
    int widthRegenerateButton = buttonFont.stringWidth("_Retransform (T)_");
    int widthLevelEditorButton = buttonFont.stringWidth("_Level Editor_");
    int widthExploitationButton = buttonFont.stringWidth("_Transformer_");
    int widthInspirationButton = buttonFont.stringWidth("_Inspiration_");
    int heightButton = buttonFont.stringHeight("_(<)_")*2;
    int displaySize = MIN(ofGetHeight(), ofGetWidth());
    
    
    int xLeftArrow = heightButton/3.;
    int xRightArrow = xLeftArrow+widthArrowButton+heightButton/3.;
    //int xPlus = xRightArrow+widthArrowButton+heightButton/3.;
    int xLevelEditor = xRightArrow+widthArrowButton+heightButton/3.;
    int xExploitation = xLevelEditor+widthLevelEditorButton+heightButton/3.; //ofGetWidth()*(1.-ideFactor)-heightButton/3. -widthInspirationButton;


    int xRegenerate = ofGetWidth()*(1.-ideFactor)-heightButton/3. - widthRegenerateButton;
    int xUndo = xRegenerate - heightButton/3. - widthUndoButton;
    int xPlay = xUndo - heightButton/3. - widthPlayButton;
    
    //int xInspiration = xxx - heightButton/3. - widthLevelEditorButton;
    
    ofSetColor(colors::colorIDE_BACKGROUND);
    if(gbl::currentGame.currentLevelIndex == 0) ofSetColor(0x55);
    if(ofGetAppPtr()->mouseX >= xLeftArrow && ofGetAppPtr()->mouseX <= xLeftArrow + widthArrowButton
       && ofGetAppPtr()->mouseY >= heightButton/3. && ofGetAppPtr()->mouseY <= heightButton/3. + heightButton && gbl::currentGame.currentLevelIndex > 0) {
        ofSetColor(0x67,0x6A,0x71);
        if(gbl::isFirstMousePressed && !activeIDE) {
            keyHandling::keyQueue.push({KEY_LEFT,0});
        }
    }
    ofDrawRectRounded(xLeftArrow, heightButton/3., widthArrowButton, heightButton, displaySize/200);
    ofSetColor(0xff);
    buttonFont.drawString(" <- ", xLeftArrow, heightButton);
    
    ofSetColor(colors::colorIDE_BACKGROUND);
    if(gbl::currentGame.currentLevelIndex+1 == gbl::currentGame.levels.size()) ofSetColor(0x55);
    if(ofGetAppPtr()->mouseX >= xRightArrow && ofGetAppPtr()->mouseX <= xRightArrow+widthArrowButton
       && ofGetAppPtr()->mouseY >= heightButton/3. && ofGetAppPtr()->mouseY <= heightButton/3. + heightButton
       && gbl::currentGame.currentLevelIndex+1 < gbl::currentGame.levels.size()) {
        ofSetColor(0x67,0x6A,0x71);
        if(gbl::isFirstMousePressed && !activeIDE) {
            keyHandling::keyQueue.push({KEY_RIGHT,0});
        }
    }
    ofDrawRectRounded(xRightArrow, heightButton/3., widthArrowButton, heightButton, displaySize/200);
    ofSetColor(0xff);
    buttonFont.drawString(" -> ", xRightArrow, heightButton);

    //ofDrawRectRounded(heightButton/3.+widthPlusButton+heightButton/3., heightButton/3., widthArrowButton, heightButton, displaySize/200);
    
    /*
    ofSetColor(0xff);
    buttonFont.drawString(" + ", heightButton/3., heightButton);
    buttonFont.drawString(" <- ", heightButton/3. + widthPlusButton+heightButton/3., heightButton);
    buttonFont.drawString(" -> ", heightButton/3.+widthPlusButton+heightButton/3.+widthArrowButton + heightButton/3., heightButton);
    */
    //buttonFont.drawString(" (+) ", heightButton/3. + widthButton + heightButton/3., heightButton);
    
    
    
    ofSetColor(colors::colorIDE_BACKGROUND);
    if(ofGetAppPtr()->mouseX >= xPlay && ofGetAppPtr()->mouseX <= xPlay+widthPlayButton
       && ofGetAppPtr()->mouseY >= heightButton/3. && ofGetAppPtr()->mouseY <= heightButton/3. + heightButton) {
        ofSetColor(0x67,0x6A,0x71);
        if(gbl::isFirstMousePressed && !activeIDE && keyHandling::keyQueue.empty()) {
            keyHandling::keyQueue.push({KEY_RESTART,0});
        }
    }
    ofDrawRectRounded(xPlay, heightButton/3., widthPlayButton, heightButton, displaySize/200);
    ofSetColor(0xff);
    buttonFont.drawString(" Play (R) ", xPlay, heightButton);
    
    ofSetColor(colors::colorIDE_BACKGROUND);
    if(gbl::record.editorHistory.size()<2) ofSetColor(0x55);
    if(gbl::record.editorHistory.size()>=2 && ofGetAppPtr()->mouseX >= xUndo && ofGetAppPtr()->mouseX <= xUndo+widthUndoButton
       && ofGetAppPtr()->mouseY >= heightButton/3. && ofGetAppPtr()->mouseY <= heightButton/3. + heightButton) {
        ofSetColor(0x67,0x6A,0x71);
        if(gbl::isFirstMousePressed && !activeIDE && keyHandling::keyQueue.empty()) {
            keyHandling::keyQueue.push({KEY_UNDO,0});
            //switchToLeftEditor(MODE_LEVEL_EDITOR);
        }
    }
    ofDrawRectRounded(xUndo, heightButton/3., widthUndoButton, heightButton, displaySize/200);
    ofSetColor(0xff);
    buttonFont.drawString(" Undo (U) ", xUndo, heightButton);
    
    ofSetColor(colors::colorIDE_BACKGROUND);
    if(ofGetAppPtr()->mouseX >= xRegenerate && ofGetAppPtr()->mouseX <= xRegenerate+widthRegenerateButton
       && ofGetAppPtr()->mouseY >= heightButton/3. && ofGetAppPtr()->mouseY <= heightButton/3. + heightButton) {
        ofSetColor(0x67,0x6A,0x71);
        if(gbl::isFirstMousePressed && !activeIDE && keyHandling::keyQueue.empty()) {
            keyHandling::keyQueue.push({KEY_GENERATE,0});
        }
    }
    ofDrawRectRounded(xRegenerate, heightButton/3., widthRegenerateButton, heightButton, displaySize/200);
    ofSetColor(0xff);
    buttonFont.drawString(" Retransform (T) ", xRegenerate, heightButton);
    
    ofSetColor(colors::colorLEVEL_EDITOR_BG);
    if(gbl::mode == MODE_LEVEL_EDITOR) ofSetColor(0x55);
    if(gbl::mode != MODE_LEVEL_EDITOR && ofGetAppPtr()->mouseX >= xLevelEditor && ofGetAppPtr()->mouseX <= xLevelEditor+widthLevelEditorButton
       && ofGetAppPtr()->mouseY >= heightButton/3. && ofGetAppPtr()->mouseY <= heightButton/3. + heightButton) {
        ofSetColor(0x67,0x6A,0x71);
        if(gbl::isFirstMousePressed && !activeIDE) {
            switchToLeftEditor(MODE_LEVEL_EDITOR, "switch_to_level_editor");
            editor::showGenerate = 0;
        }
    }
    ofDrawRectRounded(xLevelEditor, heightButton/3., widthLevelEditorButton, heightButton, displaySize/200);
    ofSetColor(0xff);
    buttonFont.drawString(" Level Editor ", xLevelEditor, heightButton);
    
    ofSetColor(colors::colorEXPLOITATION_BG);
    if(gbl::mode == MODE_EXPLOITATION) ofSetColor(0x55);
    if(gbl::mode != MODE_EXPLOITATION && ofGetAppPtr()->mouseX >= xExploitation && ofGetAppPtr()->mouseX <= xExploitation+widthExploitationButton
       && ofGetAppPtr()->mouseY >= heightButton/3. && ofGetAppPtr()->mouseY <= heightButton/3. + heightButton) {
        ofSetColor(0x67,0x6A,0x71);
        if(gbl::isFirstMousePressed && !activeIDE) {
            switchToLeftEditor(MODE_EXPLOITATION,"switch_to_transform_editor");
            if(editor::showGenerate == 0)
                editor::showGenerate = 1;
        }
    }
    ofDrawRectRounded(xExploitation, heightButton/3., widthExploitationButton, heightButton, displaySize/200);
    ofSetColor(0xff);
    buttonFont.drawString(" Transformer ", xExploitation, heightButton);
    
    
    
    if(gbl::mode == MODE_EXPLOITATION) {
        /*
         FOR THE USER STUDY IMPORTANT BUTTONS, will later be removed.
         */
        int widthWallsButton = buttonFont.stringWidth("_Add/Remove Walls_");
        int widthSpawnButton = buttonFont.stringWidth("_Add/Remove Crate Target Pairs_");
        int widthMoveButton = buttonFont.stringWidth("_Move Character/Crates_");
        int widthHybridButton = buttonFont.stringWidth("_Do a bit of everything_");

        int xWalls = ofGetWidth()*(1.-ideFactor)-heightButton/3. -widthWallsButton;
        int xSpawn = ofGetWidth()*(1.-ideFactor)-heightButton/3. -widthSpawnButton;
        int xMove = ofGetWidth()*(1.-ideFactor)-heightButton/3. -widthMoveButton;
        int xHybrid = xMove-heightButton/3. -widthHybridButton;

        int yWalls = ofGetHeight()-3*(1+1./3)*heightButton;
        int ySpawn = ofGetHeight()-2*(1+1./3)*heightButton;
        int yMove  = ofGetHeight()-1*(1+1./3)*heightButton;
        int yHybrid = ofGetHeight()-1*(1+1./3)*heightButton;

        
        static vector<string> moveStr = {
            "==========",
            "TRANSFORM",
            "==========",
            "",
            "(move players and crates around)",
            "choose 20 [Player | No Obstacle] -> [ | Player]",
            "or [Crate | No Obstacle] -> [ | Crate]"
        };
        
        static vector<string> wallsStr = {
            "==========",
            "TRANSFORM",
            "==========",
            "",
            "(randomly remove or add 20 walls with prob. 0.4)",
            "choose 20 option 0.4 [Wall] -> []",
            "or option 0.6 [No Obstacle] -> [Wall]"
        };
        
        static vector<string> spawnStr = {
            "==========",
            "TRANSFORM",
            "==========",
            "",
            "(remove one crate/target pair if it exists and add one)",
            "choose 1 [Crate][Target] -> [][]",
            "choose 1 [No Wall No Player No Crate][No Wall No Player No Target] -> [Crate][Target]",
        };
        
        static vector<string> hybridStr = {
            "==========",
            "TRANSFORM",
            "==========",
            "(move players and crates around)",
            "choose 20 [Player | No Obstacle] -> [ | Player]",
            "or [Crate | No Obstacle] -> [ | Crate]",
            "",
            "(randomly remove or add 10 walls with prob. 0.4)",
            "choose 5 option 0.5 [Wall] -> []",
            "choose 5 option 0.5 [No Obstacle] -> [Wall]"
            "",
            "(remove one crate/target pair if it exists and add one)",
            "choose 1 [Crate][Target] -> [][]",
            "choose 1 [No Wall No Player No Crate][No Wall No Player No Target] -> [Crate][Target]",
        };
        
        ofSetColor(colors::colorIDE_BACKGROUND);
        if(ideString == moveStr) ofSetColor(0x55);
        if(ofGetAppPtr()->mouseX >= xMove && ofGetAppPtr()->mouseX <= xMove+widthMoveButton
           && ofGetAppPtr()->mouseY >= yMove && ofGetAppPtr()->mouseY <= yMove + heightButton) {
            ofSetColor(0x67,0x6A,0x71);
            if(gbl::isFirstMousePressed && keyHandling::keyQueue.empty()) {
                ideString = moveStr;
                switchToLeftEditor(gbl::mode,"move_transformer");
            }
        }
        ofDrawRectRounded(xMove, yMove, widthMoveButton, heightButton, displaySize/200);
        ofSetColor(0xff);
        buttonFont.drawString(" Move Character/Crates ", xMove, yMove+heightButton*2./3);
        
        ofSetColor(colors::colorIDE_BACKGROUND);
        if(ideString == wallsStr) ofSetColor(0x55);
        if(ofGetAppPtr()->mouseX >= xWalls && ofGetAppPtr()->mouseX <= xWalls+widthWallsButton
           && ofGetAppPtr()->mouseY >= yWalls && ofGetAppPtr()->mouseY <= yWalls + heightButton) {
            ofSetColor(0x67,0x6A,0x71);
            if(gbl::isFirstMousePressed && keyHandling::keyQueue.empty()) {
                ideString = wallsStr;
                switchToLeftEditor(gbl::mode,"walls_transformer");
            }
        }
        ofDrawRectRounded(xWalls, yWalls, widthWallsButton, heightButton, displaySize/200);
        ofSetColor(0xff);
        buttonFont.drawString(" Add/Remove Walls ", xWalls, yWalls+heightButton*2./3);
        
        ofSetColor(colors::colorIDE_BACKGROUND);
        if(ideString == spawnStr) ofSetColor(0x55);
        if(ofGetAppPtr()->mouseX >= xSpawn && ofGetAppPtr()->mouseX <= xSpawn+widthSpawnButton
           && ofGetAppPtr()->mouseY >= ySpawn && ofGetAppPtr()->mouseY <= ySpawn + heightButton) {
            ofSetColor(0x67,0x6A,0x71);
            if(gbl::isFirstMousePressed && keyHandling::keyQueue.empty()) {
                ideString = spawnStr;
                switchToLeftEditor(gbl::mode,"spawn_transformer");
            }
        }
        ofDrawRectRounded(xSpawn, ySpawn, widthSpawnButton, heightButton, displaySize/200);
        ofSetColor(0xff);
        buttonFont.drawString(" Add/Remove Crate Target Pairs ", xSpawn, ySpawn+heightButton*2./3);
        
        ofSetColor(colors::colorIDE_BACKGROUND);
        if(ideString == hybridStr) ofSetColor(0x55);
        if(ofGetAppPtr()->mouseX >= xHybrid && ofGetAppPtr()->mouseX <= xHybrid+widthHybridButton
           && ofGetAppPtr()->mouseY >= yHybrid && ofGetAppPtr()->mouseY <= yHybrid + heightButton) {
            ofSetColor(0x67,0x6A,0x71);
            if(gbl::isFirstMousePressed && keyHandling::keyQueue.empty()) {
                ideString = hybridStr;
                switchToLeftEditor(gbl::mode,"hybrid_transformer");
            }
        }
        ofDrawRectRounded(xHybrid, yHybrid, widthHybridButton, heightButton, displaySize/200);
        ofSetColor(0xff);
        buttonFont.drawString(" Do a bit of everything ", xHybrid, yHybrid+heightButton*2./3);
    }
    
    
    /*
    ofSetColor(colors::colorINSPIRATION_BG);
    if(ofGetAppPtr()->mouseX >= xInspiration && ofGetAppPtr()->mouseX <= xInspiration+widthInspirationButton
       && ofGetAppPtr()->mouseY >= heightButton/3. && ofGetAppPtr()->mouseY <= heightButton/3. + heightButton) {
        ofSetColor(0x67,0x6A,0x71);
        if(gbl::isFirstMousePressed && !activeIDE) {
            switchToLeftEditor(MODE_INSPIRATION);
        }
    }
    ofDrawRectRounded(xInspiration, heightButton/3., widthInspirationButton, heightButton, displaySize/200);
    ofSetColor(0xff);
    buttonFont.drawString(" Inspiration ", xInspiration, heightButton);
    */
    

    float offsetY = heightButton/3. + heightButton + heightButton/3., offsetX = 0;
    float widthOfGame = ofGetWidth()*(1.-ideFactor), heightOfGame = ofGetHeight()-scaleBlockSelection-2*scaleBlockSelectionFont-offsetY;
    int min_field_size = 5;
    if((gbl::mode == MODE_LEVEL_EDITOR || gbl::mode == MODE_EXPLOITATION) && showGenerate > 0) {
        heightOfGame *= 4/7.;
    }
    heightOfGame += -heightButton-2*heightButton/3.;

	{ //DISPLAY LEVEL AND INFORMATION ABOUT THE LEVEL
		displayLevel(gbl::currentGame.levels[gbl::currentGame.currentLevelIndex], gbl::currentGame, offsetX, offsetY, widthOfGame, heightOfGame, min_field_size);

		string displayStrL, displayStrR = "";

		string questionMarkStr = ofGetElapsedTimeMicros() % 750000 < 250000 ? ".  " : ofGetElapsedTimeMicros() % 750000 < 500000 ? ".. " : "...";

		Game game_ = gbl::currentGame;
		/*
		uint64_t testhash1 = gbl::currentGame.getHash();
		uint64_t testhash2 = INITIAL_HASH;
		HashVVV(game_.beginStateAfterStationaryMove, testhash2);*/

		uint64_t solhash = game_.getHash(); HashVVV(game_.beginStateAfterStationaryMove, solhash);
		/*cout << "OKOK: " << solhash << " " << game_.getHash() << endl;
		for(const auto & a : game_.beginStateAfterStationaryMove) {
			for(const auto & b : a) {
				for(const auto & c : b)
					cout << c << " ";
				cout << endl;
			}
		}*/
		/*
		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 0 0 0 0 0 0
		 0 0 2 0 0 0
		 0 0 0 0 0 0
		 0 2 0 0 0 0
		 0 0 0 0 0 0
		 0 0 0 0 0 0
		 0 0 0 0 0 0
		 3 3 3 3 0 0
		 3 0 0 3 0 0
		 3 0 0 3 3 3
		 3 5 4 0 0 3
		 3 0 0 5 0 3
		 3 0 0 3 3 3
		 3 3 3 3 0 0

		 vs.

		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 1 1 1 1 1 1
		 0 0 0 0 0 0
		 0 0 2 0 0 0
		 0 0 0 0 0 0
		 0 2 0 0 0 0
		 0 0 0 0 0 0
		 0 0 0 0 0 0
		 0 0 0 0 0 0
		 3 3 3 3 0 0
		 3 0 0 3 0 0
		 3 0 0 3 3 3
		 3 5 4 0 0 3
		 3 0 0 5 0 3
		 3 0 0 3 3 3
		 3 3 3 3 0 0
		 */
		SolveInformation info;
		bool found = false;
		synchronized(solver::solutionMutex) {
			std::atomic_thread_fence(std::memory_order_seq_cst);
			if (solver::solutionDP.count(solhash) != 0) {
				info = solver::solutionDP.at(solhash);
				found = true;
			}
			std::atomic_thread_fence(std::memory_order_seq_cst);
		}

		//cerr << found << " " << info.heuristicSolver << " " << info.astarSolver
        
        if(info.success == 0) {
            displayStrL = "Unsolvable!!!";
        } else if(info.success == 2) {
            if(info.layersWithoutSolution <= 1)
                displayStrL = "No solution found yet" + questionMarkStr;
            else
                displayStrL = "No solution found within " + to_string(info.layersWithoutSolution) + " steps" + questionMarkStr;
        } else {
            if(info.shortestSolutionPath.size() != 0) {
                displayStrL = "Shortest solution size: " + to_string(info.shortestSolutionPath.size() );
            } else {
                displayStrL = "Shortest solution size lies within "
                + (info.layersWithoutSolution == -1 ? "1" : to_string(info.layersWithoutSolution))
                + "-" + to_string(info.solutionPath.size() ) + " moves.";
            }
            
            
            int chooseMin = 0, numExplored = info.statesExploredBFS > -1 ? info.statesExploredBFS : 2147483647;
            if(info.statesExploredHeuristic > -1 && info.statesExploredHeuristic < numExplored) {
                numExplored = info.statesExploredHeuristic;
                chooseMin = 1;
            }
            
            if(info.statesExploredAStar > -1 && info.statesExploredAStar < numExplored) {
                numExplored = info.statesExploredAStar;
                chooseMin = 2;
            }
            
            if(numExplored == 2147483647) displayStrR = "Difficulty " + questionMarkStr;
            else if(chooseMin == 0) displayStrR = "Difficulty " + to_string(numExplored) +" (BFS)";
            else if(chooseMin == 1) displayStrR = "Difficulty " + to_string(numExplored) +" (Greedy)";
            else if(chooseMin == 2) displayStrR = "Difficulty " + to_string(numExplored) +" (AStar)";
            
            displayStrR = "Difficulty "
            + (info.statesExploredHeuristic == -1 ? questionMarkStr : to_string(info.statesExploredHeuristic)) + " (Greedy) "
            + (info.statesExploredAStar == -1 ? questionMarkStr : to_string(info.statesExploredAStar)) + " (AStar) "
            + (info.statesExploredBFS == -1 ? questionMarkStr : to_string(info.statesExploredBFS)) + " (BFS) ";
            /*
             
             ##########
             ######..##
             #.*..*..##
             #.#..O#.##
             #..O.#OP.#
             ##*#.@...#
             ##...#####
             ##########
             */
            /*
            0 = BFS, 1 = Greedy, 2 = AStar
            displayStrR //add complexity metrics
            ="Greedy: " + (info.statesExploredHeuristic == -1 ? questionMarkStr :to_string(info.statesExploredHeuristic))
            + "     AStar: " + (info.statesExploredAStar == -1 ? questionMarkStr :to_string(info.statesExploredAStar))
            + "     BFS: " + (info.statesExploredBFS == -1 ? questionMarkStr :to_string(info.statesExploredBFS));
            */
        }
        
    
        
        int widthGenerateButton = showGenerate == 0 ? buttonFont.stringWidth("_Show Transforms_") : showGenerate == 1 ? buttonFont.stringWidth("_Show Statistics_") : buttonFont.stringWidth("_Hide Transforms_");

        ofFill();
        ofSetColor(0);
        buttonFont.drawString(displayStrL, offsetX + (widthOfGame/2. - widthGenerateButton/2. - buttonFont.stringWidth(displayStrL))/2., offsetY+heightOfGame+heightButton);
        buttonFont.drawString(displayStrR, offsetX + widthOfGame/2. + widthGenerateButton/2. + (widthOfGame/2. - widthGenerateButton/2. - buttonFont.stringWidth(displayStrR))/2., offsetY+heightOfGame+heightButton);

        
        ofSetColor(colors::colorIDE_BACKGROUND);
        if(ofGetAppPtr()->mouseX >= offsetX + widthOfGame/2. - widthGenerateButton/2. && ofGetAppPtr()->mouseX <= offsetX + widthOfGame/2. + widthGenerateButton/2.
           && ofGetAppPtr()->mouseY >= offsetY+heightOfGame + heightButton/3. && ofGetAppPtr()->mouseY <= offsetY+heightOfGame + heightButton/3.+heightButton) {
            ofSetColor(0x67,0x6A,0x71);
            if(gbl::isFirstMousePressed && !activeIDE) {
                showGenerate = (showGenerate+1)%3;
                gbl::isFirstMousePressed = false;
                gbl::isMousePressed = false;
            }
        }
        ofDrawRectRounded(offsetX + widthOfGame/2. - widthGenerateButton/2., offsetY+heightOfGame + heightButton/3., widthGenerateButton, heightButton, displaySize/200);
        ofSetColor(0xff);
        buttonFont.drawString(showGenerate == 0 ? " Show Transforms " : showGenerate == 1 ? " Show Statistics " : " Hide Transforms ", offsetX + widthOfGame/2. - widthGenerateButton/2., offsetY+heightOfGame+heightButton);
        

        //        if(info.statesExploredAStar != -1)
    }
    
    
    if(gbl::mode == MODE_EXPLOITATION) { //DRAW THE MODIFY RECTANGLES
        float scaleY = (float)heightOfGame / MAX(min_field_size, modifyTable[gbl::currentGame.currentLevelIndex]   .size());
        float scaleX = (float)widthOfGame / MAX(min_field_size, modifyTable[gbl::currentGame.currentLevelIndex][0].size());
        float scale = MIN(scaleY, scaleX);
        float centerOffsetY = (heightOfGame - scale * modifyTable[gbl::currentGame.currentLevelIndex]   .size())/2;
        float centerOffsetX = (widthOfGame  - scale * modifyTable[gbl::currentGame.currentLevelIndex][0].size())/2;
        
        ofFill();
        ofSetColor(0x0,0xff,0x0,0x88);//0xAA);
        for(int y=0;y<modifyTable[gbl::currentGame.currentLevelIndex].size();++y) {
            for(int x=0;x<modifyTable[gbl::currentGame.currentLevelIndex][y].size();++x) {
                if(modifyTable[gbl::currentGame.currentLevelIndex][y][x]) {
                    ofDrawRectangle(offsetX+centerOffsetX+x*scale, offsetY+centerOffsetY+y*scale, scale, scale);
                }
            }
        }
    }
    
    if((gbl::mode == MODE_LEVEL_EDITOR || gbl::mode == MODE_EXPLOITATION) && showGenerate > 0) {
        if(showGenerate == 2)
            heightOfGame += heightButton*2; //add some space for statistics
        
        set<pair<float,vvvs> > neighborhoodLevels;
        int generatedLevels, solvedLevels, unsolvableLevels, timedoutLevels, maxSolveTime;
        synchronized(generator::generatorMutex) {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            neighborhoodLevels = generator::generatorNeighborhood[gbl::currentGame.currentLevelIndex];
            std::atomic_thread_fence(std::memory_order_seq_cst);
            generatedLevels = generator::counter;
            solvedLevels = generator::solvedCounter;
            unsolvableLevels = generator::unsolvableCounter;
            timedoutLevels = generator::timedoutCounter;
            maxSolveTime = generator::maxSolveTime;
        }
        float offsetY2 = heightButton/3. + heightButton + heightButton/3. + heightOfGame + heightButton/3. + heightButton +heightButton/3. , offsetX2 = 0;
        float widthChangeOfGame2 = (ofGetWidth()*(1.-ideFactor))/4.;
        float widthOfGame2 = (ofGetWidth()*(1.-ideFactor))/4. - heightButton/3.;
        float heightOfGame2 = (ofGetHeight()-scaleBlockSelection-2*scaleBlockSelectionFont-offsetY2 - heightButton/6. -heightButton/2.);
        
        //for(int i=0;i<gbl::currentGame.generatorSuggestion[gbl::currentGame.currentLevelIndex].size();++i) {
        int counter = 0;
        for(const auto & nlevel : neighborhoodLevels) {
            if(++counter>4) break;
            if(ofGetAppPtr()->mouseX >= offsetX2 && ofGetAppPtr()->mouseX <= offsetX2+widthOfGame2 && ofGetAppPtr()->mouseY >=offsetY2 && ofGetAppPtr()->mouseY <= offsetY2+heightOfGame2) {
                ofFill();
                ofSetColor(0,0,0xff);
                if(gbl::isFirstMousePressed) {
                    //Check if legend contains a block not specified
                    string cstring = "";
                    if(gbl::mode == MODE_EXPLOITATION) {
                        changeLevelStr(editor::levelEditorString, gbl::currentGame.currentLevelIndex, nlevel.second,gbl::currentGame.currentState, gbl::currentGame);
                    } else if(gbl::mode == MODE_LEVEL_EDITOR) {
                        changeLevelStr(editor::ideString, gbl::currentGame.currentLevelIndex, nlevel.second,gbl::currentGame.currentState, gbl::currentGame);
                    }
                    //refreshIDEStrings(gbl::mode,gbl::mode);
                    gbl::currentGame.updateLevelState(nlevel.second, gbl::currentGame.currentLevelIndex);
                    //switchToLeftEditor(MODE_EXPLOITATION); //would force restart every suggestion

                    pushEditorState(gbl::record, "transformer_pick");
                    ofSetColor(0xff,0,0);
                    break; //since selected the generator might've changed the neighborhood
                }
                ofDrawRectangle(offsetX2 - heightButton/6., offsetY2 - heightButton/6., widthOfGame2 + heightButton/3., heightOfGame2+ heightButton/3.);
            }
            displayLevel(nlevel.second, gbl::currentGame, offsetX2, offsetY2, widthOfGame2, heightOfGame2-heightButton/2.,min_field_size);
            ofSetColor(0);
            string complexityInfoStr = "Difficulty: " + to_string((long long) -nlevel.first );
            buttonFont.drawString(complexityInfoStr, offsetX2 + (widthOfGame2 - buttonFont.stringWidth(complexityInfoStr))/2.,offsetY2+heightOfGame2);
            offsetX2 += widthChangeOfGame2;
        }
        
        if(showGenerate == 2) {
            ofSetColor(0);
            string generatedStr = "";
            generatedStr += "Solved:" + to_string(solvedLevels);
            generatedStr += "\tTimedout: " + to_string(timedoutLevels);
            generatedStr += "\tUnsolvable: " + to_string(unsolvableLevels);
            generatedStr += "\tGenerated: " + to_string(generatedLevels);
            generatedStr += "\tTimeout limit(ms): " + to_string(maxSolveTime);
            int widthOfGeneratedStr = buttonFont.stringWidth(generatedStr);
            buttonFont.drawString(generatedStr, offsetX + widthOfGame/2.-widthOfGeneratedStr/2.,offsetY2-heightButton);
        }
        
        if(neighborhoodLevels.size() <= 3 && stillTransforming()) {
            string questionMarkStr = ofGetElapsedTimeMicros() % 750000 < 250000 ? ".  " : ofGetElapsedTimeMicros() % 750000 < 500000 ? ".. " : "...";
            string transformingStr = "Transforming " + questionMarkStr;
;
            ofSetColor(0);
            buttonFont.drawString(transformingStr, offsetX2 + (widthOfGame2 - buttonFont.stringWidth(transformingStr))/2.,offsetY2+heightOfGame2/2.);
        } else if(!stillTransforming() && neighborhoodLevels.size() <= 1){
            string transformingStr = "Click Retransform (T) to start transformer.";
            ;
            ofSetColor(0);
            buttonFont.drawString(transformingStr, offsetX2 + (widthOfGame2 - buttonFont.stringWidth(transformingStr))/2.,offsetY2+heightOfGame2/2.);
        }
    }
    
    
    if(activeIDE) {
        ofFill();
        ofSetColor(0xAAAAAA,0x90);
        ofDrawRectangle(0,0,ofGetWidth()*(1.-ideFactor),ofGetHeight());
        
        if(ideLastErrorStr.size() > 0) {
            ofSetColor(0xff,0xff,0xff);
            ofDrawRectangle(0, ofGetHeight()/2. - 2*lineHeight, ofGetWidth()-ideWidth, 4*lineHeight);
            ofSetColor(0xff,0,0);
            ofFill();
            ideFont.drawString(ideLastErrorStr, 10, ofGetHeight()/2.);
        }
        
    } else {
        ofFill();
        ofSetColor(0xAAAAAA,0x90);
        ofDrawRectangle(ofGetWidth()-ideWidth,0,ideWidth,ofGetHeight());
        
        float scaleY = (float)heightOfGame / MAX(min_field_size, gbl::currentGame.levels[gbl::currentGame.currentLevelIndex][0]   .size());
        float scaleX = (float)widthOfGame / MAX(min_field_size, gbl::currentGame.levels[gbl::currentGame.currentLevelIndex][0][0].size());
        float scale = MIN(scaleY, scaleX);
        float centerOffsetY = (heightOfGame - scale * gbl::currentGame.levels[gbl::currentGame.currentLevelIndex][0]   .size())/2;
        float centerOffsetX = (widthOfGame  - scale * gbl::currentGame.levels[gbl::currentGame.currentLevelIndex][0][0].size())/2;
        
        for(int y=0;y<gbl::currentGame.levels[gbl::currentGame.currentLevelIndex][0].size();++y) {
            if(ofGetAppPtr()->mouseY >= offsetY+centerOffsetY+y*scale && ofGetAppPtr()->mouseY < offsetY+centerOffsetY+(y+1)*scale) {
                for(int x=0;x<gbl::currentGame.levels[gbl::currentGame.currentLevelIndex][0][y].size();++x) {
                    if(ofGetAppPtr()->mouseX >= offsetX+centerOffsetX+x*scale && ofGetAppPtr()->mouseX < offsetX+centerOffsetX+(x+1)*scale) {
                        if(gbl::isMousePressed && gbl::mode == MODE_LEVEL_EDITOR && selectedBlock > 0) {
                            
                            vvvs newState = gbl::currentGame.levels[gbl::currentGame.currentLevelIndex];
                            
                            for(int l=0; l<gbl::currentGame.layerCount; ++l)
                                newState[ l ][y][x] = 0;
                            
                            newState[ gbl::currentGame.objLayer [ gbl::currentGame.synonyms["background"] ] ][y][x]=gbl::currentGame.synonyms["background"];
                            if(selectedBlock < gbl::currentGame.objLayer.size()) {
                                newState[ gbl::currentGame.objLayer[ selectedBlock ] ][y][x] = selectedBlock;
                            } else {
                                for(short objId : gbl::currentGame.aggsWithSingleCharName[ selectedBlock-gbl::currentGame.objLayer.size() ].second) {
                                    short objLayer = gbl::currentGame.objLayer[objId];
                                    newState[objLayer][y][x] = objId;
                                }
                            }
                            assert(gbl::mode == MODE_LEVEL_EDITOR);
                            changeLevelStr(ideString, gbl::currentGame.currentLevelIndex, newState, gbl::currentGame.currentState, gbl::currentGame);
                            
                            gbl::currentGame.updateLevelState(newState, gbl::currentGame.currentLevelIndex);
                            
                            switchToLeftEditor(MODE_LEVEL_EDITOR, "level_editor_place");
                            
                            startGenerating();
                            //switchIDEStringAndMode(MODE_LEVEL_EDITOR);
                            //pushUndoLevelEditor("block edit");
                        }
                        if(gbl::isMousePressed && gbl::mode == MODE_EXPLOITATION && selectedExploitationTool >= 0) {
                            modifyTable[gbl::currentGame.currentLevelIndex][y][x] = (selectedExploitationTool == 1);
                            switchToLeftEditor(MODE_EXPLOITATION, "transformer_place");
                            //pushUndoLevelEditor("modify edit");
                        }
                    }
                }
            }
        }
    }
}


void ideKeyPressed(int key, bool isSuperKey, bool isAltKey, bool isShiftKey) {
    cout << "key pressed " << key << " is super: " << isSuperKey << " is alt: "<< isAltKey << " is shift " << isShiftKey << endl;
	if (!isSuperKey && key >= 32 && key <= 256 && key != 127) {
        if(selectPos != cursorPos)
            ideKeyPressed(OF_KEY_BACKSPACE, false, false, false);
        
        ideString[cursorPos.first] = ideString[cursorPos.first].substr(0, cursorPos.second) + (char)key + ideString[cursorPos.first].substr(cursorPos.second);
        cursorPos.second++;
        selectPos = cursorPos;
    } else {
        
        switch(key) {
            case OF_KEY_RETURN: // enter macOS
            {
                if(selectPos != cursorPos)
                    ideKeyPressed(OF_KEY_BACKSPACE, false, false, false);
                string str1 = ideString[cursorPos.first].substr(0,cursorPos.second);
                string str2 = ideString[cursorPos.first].substr(cursorPos.second);
                
                ideString.insert(ideString.begin()+cursorPos.first+1, str2);
                ideString[cursorPos.first] = str1;
                cursorPos.first++;
                cursorPos.second = 0;
            }
                break;
            case OF_KEY_BACKSPACE: // backspace
                if(cursorPos != selectPos) {
                    auto minPos = MIN(cursorPos, selectPos);
                    auto maxPos = MAX(cursorPos, selectPos);
                    cursorPos = maxPos;
                    selectPos = minPos;
                    while(cursorPos != selectPos) {
                        if(cursorPos.first == 0 && cursorPos.second == 0) {}
                        else if(cursorPos.second == 0) {
                            cursorPos.second = ideString[cursorPos.first-1].size();
                            ideString[cursorPos.first-1] += ideString[cursorPos.first];
                            ideString.erase(ideString.begin()+cursorPos.first);
                            cursorPos.first--;
                        }
                        else {
                            ideString[cursorPos.first] = ideString[cursorPos.first].substr(0,cursorPos.second-1) + ideString[cursorPos.first].substr(cursorPos.second);
                            cursorPos.second--;
                        }
                    }
                } else {
                    if(cursorPos.first == 0 && cursorPos.second == 0) {}
                    else if(cursorPos.second == 0) {
                        cursorPos.second = ideString[cursorPos.first-1].size();
                        ideString[cursorPos.first-1] += ideString[cursorPos.first];
                        ideString.erase(ideString.begin()+cursorPos.first);
                        cursorPos.first--;
                    }
                    else {
                        ideString[cursorPos.first] = ideString[cursorPos.first].substr(0,cursorPos.second-1) +  ideString[cursorPos.first].substr(cursorPos.second);
                        cursorPos.second--;
                    }
                }
                break;
            case 46: // DELETE
            case OF_KEY_DEL:
                if(cursorPos != selectPos) {
                    auto minPos = MIN(cursorPos, selectPos);
                    auto maxPos = MAX(cursorPos, selectPos);
                    cursorPos = maxPos;
                    selectPos = minPos;
                    while(cursorPos != selectPos) {
                        if(cursorPos.first == 0 && cursorPos.second == 0) {}
                        else if(cursorPos.second == 0) {
                            cursorPos.second = ideString[cursorPos.first-1].size();
                            ideString[cursorPos.first-1] += ideString[cursorPos.first];
                            ideString.erase(ideString.begin()+cursorPos.first);
                            cursorPos.first--;
                        }
                        else {
                            ideString[cursorPos.first] = ideString[cursorPos.first].substr(0,cursorPos.second-1) + ideString[cursorPos.first].substr(cursorPos.second);
                            cursorPos.second--;
                        }
                    }
                } else {
                    if(cursorPos.second == ideString[cursorPos.first].size() && cursorPos.first != ideString.size()) {
                        ideString[cursorPos.first] += ideString[cursorPos.first+1];
                        ideString.erase(ideString.begin()+cursorPos.first+1);
                    }
                    else {
                        string lastPart = "";
                        if(cursorPos.second < ideString[cursorPos.first].size() ) lastPart = ideString[cursorPos.first].substr(cursorPos.second + 1);
                        ideString[cursorPos.first] = ideString[cursorPos.first].substr(0,cursorPos.second) +  lastPart;
                    }
                }
                break;
                
            case OF_KEY_PAGE_DOWN:
            {
                int totalDown = MIN(20, ideString.size() - 1 - selectPos.first);
                selectPos.first += totalDown;
                if(selectPos.second > ideString[selectPos.first].size()) selectPos.second = ideString[selectPos.first].size();
                
                if(!isShiftKey) cursorPos = selectPos;
                editor::offsetIDEY = -1 * (selectPos.first - 10) * editor::ideFont.getLineHeight();
                break;
            }
            case 35: // END KEY
            case OF_KEY_END:
                if(cursorPos != selectPos) selectPos = cursorPos = MIN(cursorPos, selectPos);
                if(ideString[cursorPos.first].size() <= cursorPos.second) cursorPos.second = ideString[cursorPos.first].size();
                cursorPos.first = MAX(0,(int)ideString.size() - 1);
                cursorPos.second = MAX(0,(int)ideString[cursorPos.first].size());
                selectPos = cursorPos;
                editor::offsetIDEY = -1 *((int)ideString.size() - 10) * editor::ideFont.getLineHeight();
                break;
                
                
            case OF_KEY_PAGE_UP:
            {
                int totalUp = MIN(20, selectPos.first);
                selectPos.first -= totalUp;
                if(selectPos.second > ideString[selectPos.first].size()) selectPos.second = ideString[selectPos.first].size();
                
                if(!isShiftKey) cursorPos = selectPos;
                editor::offsetIDEY = -1 * (selectPos.first - 10) * editor::ideFont.getLineHeight();
                break;
            }
            case 36: // HOME KEY
            case OF_KEY_HOME:
                if(cursorPos != selectPos) selectPos = cursorPos = MIN(cursorPos, selectPos);
                cursorPos.first = 0;
                cursorPos.second = 0;
                selectPos = cursorPos;
                editor::offsetIDEY = 0;
                break;
                
            case 356: //left windows
            case OF_KEY_LEFT: //left macOS
                if(isSuperKey) {
                    if(cursorPos != selectPos) selectPos = cursorPos = MIN(cursorPos, selectPos);
                    cursorPos.second = 0;
                }
                else if(isAltKey) {
                        if(selectPos.first > 0 && selectPos.second == 0) selectPos.second = ideString[--selectPos.first].size();
                        if(selectPos.second > 0 ) {
                            do { selectPos.second --; }
                            while(selectPos.second > 0 && ideString[selectPos.first][selectPos.second - 1] != ' ');
                        }
                        if(!isShiftKey) {
                            cursorPos = selectPos;
                        }
                } else if(isShiftKey) {
                    if(selectPos.second > 0) selectPos.second --;
                    else if(selectPos.first > 0) {
                        selectPos.first--;
                        selectPos.second = ideString[selectPos.first].size()-1;
                    }
                }
                else {
                    if(cursorPos != selectPos) selectPos = cursorPos = MIN(cursorPos, selectPos);
                    else {
                        if(cursorPos.second > 0)
                            cursorPos.second--;
                        else if(cursorPos.first > 0)
                            cursorPos.second = ideString[--cursorPos.first].size();
                    }
                }
                break;
                
            case 357: //up windows
            case OF_KEY_UP: //up macOS
                if(isSuperKey) {
                    ideKeyPressed(OF_KEY_HOME, false, false, false);
                } else if(isShiftKey) {
                    if(selectPos.first == 0) selectPos = {0,0};
                    else {
                        selectPos.first--;
                        selectPos.second = MIN(cursorPos.second, ideString[selectPos.first].size());
                    }
                } else {
                    if(cursorPos != selectPos) selectPos = cursorPos = MIN(cursorPos, selectPos);
                    else {
                        if(cursorPos.first == 0) cursorPos = {0,0};
                        if(cursorPos.first > 0) {
                            cursorPos.first--;
                            if(ideString[cursorPos.first].size() <= cursorPos.second) cursorPos.second = ideString[cursorPos.first].size();
                        }
                    }
                }
                break;
                
                
            case 358: //right windows
            case OF_KEY_RIGHT: //right macOS
                if(isSuperKey) {
                    if(cursorPos != selectPos) selectPos = cursorPos = MIN(cursorPos, selectPos);
                    cursorPos.second = MAX(0,(int)ideString[cursorPos.first].size());
                } else if(isAltKey) {
                    if(selectPos.second == ideString[selectPos.first].size() && selectPos.first+1 < ideString.size()) {
                        selectPos.first++;
                        selectPos.second = 0;
                    }
                    if(selectPos.second < ideString[selectPos.first].size()) {
                        do { selectPos.second ++;}
                        while(selectPos.second < ideString[selectPos.first].size() && ideString[selectPos.first][selectPos.second] != ' ');
                    }
                    if(!isShiftKey) {
                        cursorPos = selectPos;
                    }
                } else if(isShiftKey) {
                    if(selectPos.second < ideString[selectPos.first].size()) selectPos.second ++;
                    else if(selectPos.first < ideString.size()) {
                        selectPos.first++;
                        selectPos.second = 0;
                    }
                }
                else {
                    if(cursorPos != selectPos) selectPos = cursorPos = MAX(cursorPos, selectPos);
                    else {
                        if(cursorPos.second < ideString[cursorPos.first].size())
                            cursorPos.second++;
                        else if(cursorPos.first+1 < ideString.size()){
                            cursorPos.first++;
                            cursorPos.second = 0;
                        }
                    }
                }
                break;
                
                
            case 359: //down windows
            case OF_KEY_DOWN: //down macOS
                if(isSuperKey) {
                    ideKeyPressed(OF_KEY_PAGE_DOWN, false, false, false);
                } else if(isShiftKey) {
                    if(selectPos.first + 1 == ideString.size()) selectPos.second = ideString[selectPos.first].size();
                    else {
                        selectPos.first++;
                        selectPos.second = MIN(cursorPos.second, ideString[selectPos.first].size());
                    }
                } else {
                    if(cursorPos != selectPos) selectPos = cursorPos = MIN(cursorPos, selectPos);
                    else {
                        if(cursorPos.first+1 == ideString.size()) cursorPos = {cursorPos.first,ideString[cursorPos.first].size()};
                        else if(cursorPos.first+1 < ideString.size()) {
                            cursorPos.first++;
                            if(ideString[cursorPos.first].size() <= cursorPos.second) cursorPos.second = ideString[cursorPos.first].size();
                        }
                    }
                }
                break;
            case 97:
                if(isSuperKey) {
                    selectPos = { 0,0 };
                    cursorPos = { MAX(0,(int)ideString.size() - 1), ideString.at(ideString.size() - 1).size() };
                }
			break;
			case 22: //paste Windows if command is pressed
            case 118: //paste macOS if command is pressed (i think 22 if control is pressed)
            {
                string result = ofGetWindowPtr()->getClipboardString();
                for(unsigned char c : result) {
                    if(c != 10 && c < 32) continue;
                    if(c != 10) ideKeyPressed(c, false, false, false);
                    else ideKeyPressed(13, false, false, false);
                }
            }
            break;
			case 3: //copy Windows if command is pressed
            case 99: //copy macOS if command is pressed
            {
                //convert string into clipboard string:
                auto minPos = MIN(cursorPos, selectPos);
                auto maxPos = MAX(cursorPos, selectPos);
                if(minPos == maxPos) return; //nothing to copy
                
                string copystr = "";
                
                if(minPos.first == maxPos.first) copystr = ideString[minPos.first].substr(minPos.second, maxPos.second-minPos.second);
                else {
                    copystr += ideString[minPos.first].substr(minPos.second);
                    for(int i=minPos.first+1;i<maxPos.first;++i) {
#ifdef _WIN32
						copystr += "\r";
#endif
                        copystr += "\n";
                        copystr += ideString[i];
                    }
#ifdef _WIN32
					copystr += "\r";
#endif
                    copystr += "\n";
                    copystr += ideString[maxPos.first].substr(0, maxPos.second);
                }
                
                ofGetWindowPtr()->setClipboardString(copystr);
            }
            break;
            default:;
                cerr << "unknown keytype " << key << " " << isSuperKey << endl;
        }
        
        if(key == 13 || key == 8  || (!isShiftKey && (key == 356 || key == 357 || key == 57357 || key == 358 || key == 57358 || key == 359 || key == 57359)) || key == 118 || key == 22) {
            selectPos = cursorPos;
        }
    }
}


void displayPlayMode() {
	/*
	 ofTrueTypeFontSettings settings(possibleFonts[7],charW());
	 settings.antialiased = true;
	 settings.addRanges({
	 ofUnicode::Latin1Supplement,
	 ofUnicode::Hiragana,
	 ofUnicode::Katakana,
	 ofUnicode::CJKUnified

	 });

	 settings.addRanges(ofAlphabet::Emoji);
	 settings.addRanges(ofAlphabet::Japanese);

	 font.load(settings);
	 */

	static ofTrueTypeFont playFont;
	static ofTrueTypeFont arrowFont;
	static float prevFontWidth = -1;

	float fontWidth = MIN(ofGetWidth(), ofGetHeight()) / 40;//load all available ranges openframeworks
	static ofTrueTypeFontSettings settings(gbl::locationOfResources + "font/Fira_Mono/FiraMono-Bold.ttf", fontWidth);
	if (prevFontWidth != fontWidth) {
		cout << "Loading Font" << endl;
		settings.fontSize = fontWidth;
		settings.antialiased = true;
		settings.addRanges({
			ofUnicode::Latin,
			ofUnicode::Arrows,
			ofUnicode::MiscSymbols
			// ofUnicode::range{(uint32_t)11013,(uint32_t)11015},
			// ofUnicode::range{(uint32_t)10145,(uint32_t)10145}
			});
		arrowFont.load(settings);
		playFont.load(gbl::locationOfResources + "font/Inconsolata/Inconsolata-Regular.ttf", MIN(ofGetWidth(), ofGetHeight()) / 75);
		prevFontWidth = fontWidth;
	}

	int heightButton = playFont.stringHeight("()") * 2;
	int displaySize = MIN(ofGetHeight(), ofGetWidth());

	string displayStrTop = "Current moves: ", displayStrBot, unmatchedStr, matchedStr, playerMoveStr, unmatchedStrASCII, matchedStrASCII, playerMoveStrASCII;

	string questionMarkStr = ofGetElapsedTimeMicros() % 750000 < 250000 ? ".  " : ofGetElapsedTimeMicros() % 750000 < 500000 ? ".. " : "...";

	uint64_t solhash = gbl::currentGame.getHash(); HashVVV(gbl::currentGame.beginStateAfterStationaryMove, solhash);
	SolveInformation info;
	synchronized(solver::solutionMutex) {
		std::atomic_thread_fence(std::memory_order_seq_cst);
		if (solver::solutionDP.count(solhash) != 0) {
			info = solver::solutionDP.at(solhash);
		}
		std::atomic_thread_fence(std::memory_order_seq_cst);
	}

	deque<short> currentMoves = gbl::currentGame.getCurrentMoves();
	/*
#define addMoves(comp,toadd)\
if     (comp == UP_MOVE    ) toadd += "↑";\
else if(comp == DOWN_MOVE  ) toadd += "↓";\
else if(comp == LEFT_MOVE  ) toadd += "←";\
else if(comp == RIGHT_MOVE ) toadd += "→";\
else if(comp == ACTION_MOVE) toadd += "A";\
else assert(false);
	*/
#define addMovesASCII(comp,toadd)\
if     (comp == UP_MOVE    ) toadd += "U";\
else if(comp == DOWN_MOVE  ) toadd += "D";\
else if(comp == LEFT_MOVE  ) toadd += "L";\
else if(comp == RIGHT_MOVE ) toadd += "R";\
else if(comp == ACTION_MOVE) toadd += "A";\
else assert(false);
#define addMoves(comp,toadd)\
if (comp == UP_MOVE) toadd += "↑";\
else if (comp == DOWN_MOVE) toadd += "↓";\
else if (comp == LEFT_MOVE) toadd += "←";\
else if (comp == RIGHT_MOVE) toadd += "→";\
else if (comp == ACTION_MOVE) toadd += "A";\
else assert(false);

	for (short move : currentMoves) {
		addMoves(move, playerMoveStr);
        addMovesASCII(move, playerMoveStrASCII);
	}

	if (info.success == 0) {
		displayStrBot = "Unsolvable!!!";
	}
	else if (info.success == 2) {
		if (info.layersWithoutSolution <= 1)
			displayStrBot = "No solution found yet" + questionMarkStr;
		else
			displayStrBot = "No solution found within " + to_string(info.layersWithoutSolution) + " steps" + questionMarkStr;
	}
	else {
		if (info.shortestSolutionPath.size() != 0) {
			displayStrBot = "Shortest solution: ";
		}
		else {
			displayStrBot = "Shortest known solution: ";
		}

		//match until first string is matched
		int solutionIt = 0;
		for (;solutionIt < currentMoves.size() && solutionIt < info.solutionPath.size() && currentMoves[solutionIt] == info.solutionPath[solutionIt];++solutionIt) {
			addMoves(currentMoves[solutionIt], matchedStr);
            addMovesASCII(info.solutionPath[solutionIt], matchedStrASCII);
		}
		for (;solutionIt < info.solutionPath.size();++solutionIt) {
			addMoves(info.solutionPath[solutionIt], unmatchedStr);
            addMovesASCII(info.solutionPath[solutionIt], unmatchedStrASCII);
		}
	}


	ofFill();
	ofSetColor(0);
	playFont.drawString(displayStrTop, heightButton / 3., ofGetHeight() - (1 + 2. / 3) * heightButton);
	playFont.drawString(displayStrBot, heightButton / 3., ofGetHeight() - (0 + 2. / 3) * heightButton);
	int x2 = MAX(heightButton / 3. + playFont.stringWidth(displayStrBot + "_"), heightButton / 3. + playFont.stringWidth(displayStrTop + "_"));
	ofSetColor(0, 0, 0xff);

//#ifdef _WIN32
	playFont.drawString(matchedStrASCII, x2, ofGetHeight() - (0 + 2. / 3) * heightButton + heightButton / 12.);
	int x3 = x2 + playFont.stringWidth(matchedStrASCII + "_");
	ofSetColor(0);
	playFont.drawString(unmatchedStrASCII, x3, ofGetHeight() - (0 + 2. / 3) * heightButton + heightButton / 12.);
	int xbot = x3 + playFont.stringWidth(unmatchedStrASCII + "_");
	ofSetColor(0, 0, 0xff);
	playFont.drawString(playerMoveStrASCII, x2, ofGetHeight() - (1 + 2. / 3) * heightButton + heightButton / 12.);
	int xtop = x2 + playFont.stringWidth(playerMoveStrASCII + "_");
/*
#else
	int x3 = x2 + arrowFont.stringWidth(matchedStr+"_");
	int xbot = x3 + arrowFont.stringWidth(unmatchedStr+"_");
	int xtop = x2 + arrowFont.stringWidth(playerMoveStr + "_");

    if(xtop + playFont.stringWidth("_Try solving from here_") > ofGetWidth() || xbot + playFont.stringWidth("_Show solution_") > ofGetWidth()) {
        playFont.drawString(matchedStrASCII, x2, ofGetHeight() - (0 + 2. / 3) * heightButton + heightButton / 12.);
        x3 = x2 + playFont.stringWidth(matchedStrASCII + "_");
        ofSetColor(0);
        playFont.drawString(unmatchedStrASCII, x3, ofGetHeight() - (0 + 2. / 3) * heightButton + heightButton / 12.);
        xbot = x3 + playFont.stringWidth(unmatchedStrASCII + "_");
        ofSetColor(0, 0, 0xff);
        playFont.drawString(playerMoveStrASCII, x2, ofGetHeight() - (1 + 2. / 3) * heightButton + heightButton / 12.);
        xtop = x2 + playFont.stringWidth(playerMoveStrASCII + "_");
    } else {
        arrowFont.drawString(matchedStr, x2, ofGetHeight() - (0 + 2. / 3) * heightButton + heightButton / 12.);
        ofSetColor(0);
        arrowFont.drawString(unmatchedStr, x3, ofGetHeight() - (0 + 2. / 3) * heightButton + heightButton / 12.);
        ofSetColor(0, 0, 0xff);
        arrowFont.drawString(playerMoveStr, x2, ofGetHeight() - (1 + 2. / 3) * heightButton + heightButton / 12.);
    }
#endif
    */
    
    if(info.success == 1) { //Success
        int widthButtonShowSolution = playFont.stringWidth("_Show solution_");
        ofSetColor(colors::colorIDE_BACKGROUND);
        if(ofGetAppPtr()->mouseX >= xbot && ofGetAppPtr()->mouseX <= xbot+widthButtonShowSolution
           && ofGetAppPtr()->mouseY >= ofGetHeight() - (1+1./3) * heightButton && ofGetAppPtr()->mouseY <= ofGetHeight() - (0+1./3) * heightButton && keyHandling::keyQueue.empty()) {
            ofSetColor(0x67,0x6A,0x71);
            if(gbl::isFirstMousePressed) {
                if(gbl::currentGame.currentState != gbl::currentGame.beginStateAfterStationaryMove)
                    keyHandling::keyQueue.push({KEY_RESTART,100});
                keyHandling::keyQueue.push({KEY_SOLVE,100});
            }
        }
        ofDrawRectRounded(xbot, ofGetHeight() - (1+1./3) * heightButton, widthButtonShowSolution, heightButton, displaySize/200);
        ofSetColor(0xff);
        playFont.drawString(" Show solution ", xbot, ofGetHeight() - (0+2./3) * heightButton);
    }
    
    uint64_t solhash2 = gbl::currentGame.getHash(); HashVVV(gbl::currentGame.currentState, solhash2);
    SolveInformation infoCurrent;
    synchronized(solver::solutionMutex) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        if(solver::solutionDP.count(solhash2) != 0) {
            infoCurrent = solver::solutionDP.at(solhash2);
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    
    static vvvs hasPressedSolveFromThisState = {};
    if(info.success != 0 && infoCurrent.success == 2 && hasPressedSolveFromThisState != gbl::currentGame.currentState){
        int widthButtonSolveFromHere = playFont.stringWidth("_Try solving from here_");
        ofSetColor(colors::colorIDE_BACKGROUND);
        if(ofGetAppPtr()->mouseX >= xtop && ofGetAppPtr()->mouseX <= xtop+widthButtonSolveFromHere
           && ofGetAppPtr()->mouseY >= ofGetHeight() - (2+1./3) * heightButton && ofGetAppPtr()->mouseY <= ofGetHeight() - (1+1./3) * heightButton) {
            ofSetColor(0x67,0x6A,0x71);
            if(gbl::isFirstMousePressed) {
                hasPressedSolveFromThisState = gbl::currentGame.currentState;
                startSolving(1, gbl::currentGame.beginStateAfterStationaryMove, gbl::currentGame, currentMoves);
            }
        }
        ofDrawRectRounded(xtop, ofGetHeight() - (2+1./3) * heightButton, widthButtonSolveFromHere, heightButton, displaySize/200);
        
        ofSetColor(0xff);
        playFont.drawString(" Try solving from here ", xtop, ofGetHeight() - (1+2./3) * heightButton);
    } else if(keyHandling::keyQueue.size() == 0 && info.success != 0) { //Equal show progress and if it's solvable solve it
        if(infoCurrent.success == 1 ) {
            string foundSolutionButtonTop = "";
            if(infoCurrent.shortestSolutionPath.size() != 0) {
                foundSolutionButtonTop = "Show shortest solution from here (L)";
            } else {
                foundSolutionButtonTop = "Show possible solution from here (L)";
            }
            int widthButtonSolutionButtonTop = playFont.stringWidth("_"+foundSolutionButtonTop+"_");
            ofSetColor(colors::colorIDE_BACKGROUND);
            if(ofGetAppPtr()->mouseX >= xtop && ofGetAppPtr()->mouseX <= xtop+widthButtonSolutionButtonTop
               && ofGetAppPtr()->mouseY >= ofGetHeight() - (2+1./3) * heightButton && ofGetAppPtr()->mouseY <= ofGetHeight() - (1+1./3) * heightButton) {
                ofSetColor(0x67,0x6A,0x71);
                if(gbl::isFirstMousePressed) {
                    keyHandling::keyQueue.push({KEY_SOLVE, 100});
                }
            }
            ofDrawRectRounded(xtop, ofGetHeight() - (2+1./3) * heightButton, widthButtonSolutionButtonTop, heightButton, displaySize/200);
            ofSetColor(0xff);
            playFont.drawString(" "+foundSolutionButtonTop+" ", xtop, ofGetHeight() - (1+2./3) * heightButton);
        } else {
            string topSolveInformation = "";
            if(infoCurrent.success == 0) {
                topSolveInformation = "Unsolvable!!!";
            } else if(infoCurrent.success == 2) {
                if(infoCurrent.layersWithoutSolution <= 1)
                    topSolveInformation = "No solution found yet" + questionMarkStr;
                else
                    topSolveInformation = "No solution found within " + to_string(infoCurrent.layersWithoutSolution) + " steps" + questionMarkStr;
            }
            ofSetColor(0x0);
            playFont.drawString(topSolveInformation, xtop, ofGetHeight() - (1+2./3) * heightButton);
        }
    }
    
    int widthPlayButton = playFont.stringWidth(gbl::currentGame.beginStateAfterStationaryMove == gbl::currentGame.currentState ? "_Menu (R)_" : "_Restart (R)_");
    int widthUndoButton = playFont.stringWidth("_Undo (U)_");
    
    int xUndo = ofGetWidth() - heightButton/3. - widthUndoButton;
    int xPlay = xUndo - heightButton/3. - widthPlayButton;
    
    ofSetColor(colors::colorLEVEL_EDITOR_BG);
    
    if(ofGetAppPtr()->mouseX >= xPlay && ofGetAppPtr()->mouseX <= xPlay+widthPlayButton
       && ofGetAppPtr()->mouseY >= heightButton/3. && ofGetAppPtr()->mouseY <= heightButton/3. + heightButton) {
        ofSetColor(0x67,0x6A,0x71);
        if(gbl::isFirstMousePressed && keyHandling::keyQueue.empty()) {
            keyHandling::keyQueue.push({KEY_RESTART,0});
        }
    }
    ofDrawRectRounded(xPlay, heightButton/3., widthPlayButton, heightButton, displaySize/200);
    ofSetColor(0xff);
    playFont.drawString(gbl::currentGame.beginStateAfterStationaryMove == gbl::currentGame.currentState ? " Menu (R) " : " Restart (R) ", xPlay, heightButton);
    
    ofSetColor(colors::colorLEVEL_EDITOR_BG);
    if(gbl::currentGame.undoStates.size()<1) ofSetColor(0x55);
    if(gbl::currentGame.undoStates.size()>=1 && ofGetAppPtr()->mouseX >= xUndo && ofGetAppPtr()->mouseX <= xUndo+widthUndoButton
       && ofGetAppPtr()->mouseY >= heightButton/3. && ofGetAppPtr()->mouseY <= heightButton/3. + heightButton) {
        ofSetColor(0x67,0x6A,0x71);
        if(gbl::isFirstMousePressed && keyHandling::keyQueue.empty()) {
            keyHandling::keyQueue.push({KEY_UNDO,0});
            //switchToLeftEditor(MODE_LEVEL_EDITOR);
        }
    }
    ofDrawRectRounded(xUndo, heightButton/3., widthUndoButton, heightButton, displaySize/200);
    ofSetColor(0xff);
    playFont.drawString(" Undo (U) ", xUndo, heightButton);
    
    displayLevel(gbl::currentGame.currentState, gbl::currentGame, 0, (1+2./3)*heightButton, ofGetWidth(), ofGetHeight() - 3 * heightButton - (1+2./3)*heightButton, 5);
}

void displayLevel(const vvvs & state, const Game & game, int offsetX, int offsetY, int width, int height, int min_field_size) {
    ofSetColor(255);
    ofNoFill();
    if(state.size() == 0 || state[0].size() == 0 || state[0][0].size() == 0) return;
    
    float scaleY = (float)height / MAX(min_field_size, state[0]  .size());
    float scaleX = (float)width / MAX(min_field_size, state[0][0].size());
    float scale = MIN(scaleY, scaleX);
    float centerOffsetY = (height - scale * state[0]   .size())/2;
    float centerOffsetX = (width  - scale * state[0][0].size())/2;
    
    for(int layer=0;layer<state.size();++layer) {
        for(int y=0;y<state[layer].size();++y) {
            for(int x=0;x<state[layer][y].size();++x) {
                int objIndex = state[layer][y][x];
                if(objIndex != 0) {
                    assert(objIndex < game.objTexture.size());
                    colors::textures[ game.objTexture[objIndex] ].draw(offsetX+centerOffsetX+x*scale, offsetY+centerOffsetY+y*scale,scale,scale);
                }
            }
        }
    }
}

#endif

