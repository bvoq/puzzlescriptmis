#include "global.h"

#include "engine.h"
#include "game.h"
#include "recordandundo.h"
#include "solver.h"
#include "visualsandide.h"

namespace gbl {
    MODE_TYPE mode = MODE_LEVEL_EDITOR;
    Game currentGame;
    Record record;
    int version = 100000;
    
    const string locationOfResources = "data/";
    bool isMousePressed = false, isMouseReleased = false, isFirstMousePressed = false;
    
    const deque<short> emptyMoves = {};
}

// This is the main controller. All logic of non-local state changes happen in this file.

void switchToLevel(int level, Game & game) {
    cout << "SWITCHING TO LEVEL " << level << endl;
    //first load up all the messages
    game.currentLevelIndex = level;
    game.currentState = game.levels[level];
    game.currentLevelHeight = game.currentState[0].size();
    game.currentLevelWidth = game.currentState[0][0].size();
    
    vvvs copyOfState = game.currentState;
    moveAndChangeField(STATIONARY_MOVE, copyOfState, game);
    game.beginStateAfterStationaryMove = copyOfState;
    
    game.undoStates.clear();
    game.currentMessageIndex = 0;
    
    startSolving(0, game.beginStateAfterStationaryMove, game, gbl::emptyMoves);
    
    /* TODO:
     if(game.currentMessageIndex < game.messages[level].size()) {
     //switch into message mode
     mode = MODE_MESSAGE;
     } else {*/
    
    //mode = MODE_LEVEL_EDITOR;
}
