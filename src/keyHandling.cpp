#include "keyHandling.h"

#include "engine.h"
#include "game.h"
#include "generation.h"
#include "global.h"
#include "recordandundo.h"
#include "solver.h"
#include "visualsandide.h"

namespace keyHandling {
    queue<pair<KEY_TYPE,long long> > keyQueue; //which key should be pressed and how long the keyboard input should be paused after this.

    static map<int, KEY_TYPE> keyMapper;
    static chrono::steady_clock::time_point lastPressedTime = chrono::steady_clock::now();
}
using namespace keyHandling;


//map<KEY_TYPE, pair<long long,bool> > keyPressedDown;
//map<KEY_TYPE, bool> timeWaitForRepress;

//bool setRemapKey = false;
//keyType remapKey = UNKNOWNKEYT;

/*
 Key Codes:
 ----------
 */

void initDefaultKeyMapping() {
    keyMapper.insert({87, KEY_UP});
    keyMapper.insert({119, KEY_UP});
    keyMapper.insert({357, KEY_UP});
    keyMapper.insert({57357,KEY_UP});
    
    keyMapper.insert({65, KEY_LEFT});
    keyMapper.insert({97, KEY_LEFT});
    keyMapper.insert({356, KEY_LEFT});
    keyMapper.insert({57356, KEY_LEFT});
    
    keyMapper.insert({83, KEY_DOWN});
    keyMapper.insert({115, KEY_DOWN});
    keyMapper.insert({359, KEY_DOWN});
    keyMapper.insert({57359, KEY_DOWN });
    
    keyMapper.insert({68, KEY_RIGHT});
    keyMapper.insert({100, KEY_RIGHT});
    keyMapper.insert({358, KEY_RIGHT});
    keyMapper.insert({57358, KEY_RIGHT});
    
    keyMapper.insert({13, KEY_ACTION});
    keyMapper.insert({32, KEY_ACTION});
    
    keyMapper.insert({88, KEY_ACTION});
    keyMapper.insert({120, KEY_ACTION});
    
    keyMapper.insert({85, KEY_UNDO});
    keyMapper.insert({117, KEY_UNDO});
    
    keyMapper.insert({90, KEY_UNDO});
    keyMapper.insert({122, KEY_UNDO});
    
    keyMapper.insert({82, KEY_RESTART});
    keyMapper.insert({114, KEY_RESTART});
    
    keyMapper.insert({76, KEY_SOLVE}); //button l
    keyMapper.insert({108, KEY_SOLVE});
    
    keyMapper.insert({80, KEY_PRINT});
    keyMapper.insert({112, KEY_PRINT});
    
    keyMapper.insert({73, KEY_IMPORT});
    keyMapper.insert({105, KEY_IMPORT});
    
    keyMapper.insert({71, KEY_GENERATE});
    keyMapper.insert({103, KEY_GENERATE});
    
    keyMapper.insert({84, KEY_GENERATE});
    keyMapper.insert({116, KEY_GENERATE});
    /*
     keyMapper.insert({57, CHANGE_TO_SUPERAIR});
     
     keyMapper.insert({67, CLEAR});
     keyMapper.insert({99, CLEAR});
     
     keyMapper.insert({76, SOLVE});
     keyMapper.insert({108, SOLVE});
     
     keyMapper.insert({27, TOGGLE_TOOLBAR});
     
     keyMapper.insert({105, IMPROVE});
     */
}


void keyHandle(int key) {
    if(keyMapper.count(key) == 0) {
        if(gbl::mode == MODE_LEVEL_EDITOR && !editor::activeIDE && key >= '0' && key <= '9') {
            int selectkey = key == '0' ? 9 : (key-'1');
            if(selectkey < gbl::currentGame.synsWithSingleCharName.size()) {
                editor::selectedBlockStr = gbl::currentGame.synsWithSingleCharName[selectkey].first;
                editor::selectedBlock = gbl::currentGame.synsWithSingleCharName[selectkey].second;
            } else if(selectkey < gbl::currentGame.synsWithSingleCharName.size() + gbl::currentGame.aggsWithSingleCharName.size()) {
                editor::selectedBlockStr = gbl::currentGame.aggsWithSingleCharName[selectkey - gbl::currentGame.synsWithSingleCharName.size()].first;
                editor::selectedBlock = selectkey - gbl::currentGame.synsWithSingleCharName.size() + gbl::currentGame.objLayer.size();
            }
        }
        if(gbl::mode == MODE_EXPLOITATION && !editor::activeIDE && key >= '0' && key <= '9') {
            int selectkey = key == '0' ? 9 : (key-'1');
            if(selectkey < 2) {
                editor::selectedExploitationTool = selectkey;
            }
        }
        DEB("pressed key " + to_string(key));
    } //unknown key
    else {
        keyQueue.push({keyMapper[key],0});
    }
}



void executeKeys() {
    if(keyQueue.size()==0) return;
    
    chrono::steady_clock::time_point ctime = chrono::steady_clock::now();
    auto dur = chrono::duration_cast<std::chrono::milliseconds>(ctime - lastPressedTime).count();
    
    if(dur > keyQueue.front().second)  {
        KEY_TYPE key = keyQueue.front().first;
        keyQueue.pop();
        lastPressedTime = ctime;

        switch(key) {
            case KEY_LEFT:
            case KEY_RIGHT:
                if(gbl::mode == MODE_EXPLOITATION || gbl::mode == MODE_LEVEL_EDITOR || gbl::mode == MODE_INSPIRATION) {
                    if(key==KEY_LEFT && gbl::currentGame.currentLevelIndex > 0) gbl::currentGame.currentLevelIndex--;
                    else if(key==KEY_RIGHT && gbl::currentGame.currentLevelIndex+1<gbl::currentGame.levels.size()) gbl::currentGame.currentLevelIndex++;
                    switchToLeftEditor(gbl::mode,"switching_level_to_"+to_string(gbl::currentGame.currentLevelIndex));
                    break;
                }
            case KEY_UP:
            case KEY_DOWN:
            case KEY_ACTION:
                if(gbl::mode == MODE_PLAYING) {
                    //if there is a change add to undo map
                    short dir = key == KEY_UP ? UP_MOVE : key == KEY_DOWN ? DOWN_MOVE : key == KEY_LEFT ? LEFT_MOVE : key == KEY_RIGHT ? RIGHT_MOVE : ACTION_MOVE;
                    bool winning = move(dir, gbl::currentGame);
                    
                    if(winning) {
                        //find out the moves that let to the solution:
                        deque<short> solutionMoves = gbl::currentGame.getCurrentMoves();
                        keyQueue.push({KEY_WIN,300});
                        
                        uint64_t solhash = gbl::currentGame.getHash(); HashVVV(gbl::currentGame.beginStateAfterStationaryMove, solhash);
                        SolveInformation info;
                        info.success = 1;
                        info.solutionPath = solutionMoves;
                        
                        synchronized(solver::solutionMutex) {
                            std::atomic_thread_fence(std::memory_order_seq_cst);
                            if(solver::solutionDP.count(solhash) != 0) {
                                // cout << "READ1 " << solver::solutionDP.at(solhash).shortestSolutionPath.size() << endl;
                                info = mergeSolveInformation(solver::solutionDP.at(solhash), info);
                                // cout << "SET1: " << info.shortestSolutionPath.size() << " " << solhash << endl;
                                solver::solutionDP.at(solhash) = info;
                                // cout << "RES1: " << info.shortestSolutionPath.size() << endl;
                            } else {
                                solver::solutionDP.insert({solhash,info});
                            }
                            std::atomic_thread_fence(std::memory_order_seq_cst);
                        }
                    }
                    
                    //stopSolving(1);
                    //move(STATIONARY_MOVE, gbl::currentGame);
                }
                
                break;
            case KEY_WIN:
                if(gbl::mode == MODE_PLAYING) {
                    //gbl::mode = MODE_PLAYING
                    //switchToLevel(gbl::currentGame.currentLevelIndex+1, gbl::currentGame);
                    switchToLevel(gbl::currentGame.currentLevelIndex, gbl::currentGame);
                    switchToLeftEditor(editor::previousMenuMode,"won_level");
                }
            break;
            case KEY_UNDO:
                if(gbl::mode == MODE_PLAYING)
                    undo(gbl::currentGame);
                else if(gbl::mode == MODE_LEVEL_EDITOR || gbl::mode == MODE_EXPLOITATION) {
                    undoEditorState(gbl::record);
                }
                break;
            case KEY_RESTART:
                if(gbl::mode == MODE_PLAYING) {
                    if(gbl::currentGame.beginStateAfterStationaryMove == gbl::currentGame.currentState) {
                        //gbl::mode = MODE_LEVEL_EDITOR;
                        switchToLevel(gbl::currentGame.currentLevelIndex, gbl::currentGame);
                        switchToLeftEditor(editor::previousMenuMode,"switch_from_play_to_menu_wo_win");
                    } else {
                        restart(gbl::currentGame);
                    }
                }
                else if(gbl::mode == MODE_LEVEL_EDITOR || gbl::mode == MODE_EXPLOITATION) {
                    switchToLevel(gbl::currentGame.currentLevelIndex, gbl::currentGame);
                    gbl::mode = MODE_PLAYING;
                    //do a stationary move before starting:
                    //gbl::currentGame.currentState = gbl::currentGame.levels[gbl::currentGame.currentLevelIndex];
                    move(STATIONARY_MOVE, gbl::currentGame);
                    if(gbl::currentGame.undoStates.size() > 0) gbl::currentGame.undoStates.pop_back();
                    gbl::currentGame.beginStateAfterStationaryMove = gbl::currentGame.currentState;
                }
            
                break;
            case KEY_SOLVE:
                if(gbl::mode == MODE_PLAYING) {
                    //16255 steps A*
                    //31677 steps (28.9s) heuristicSolver
                    //253418 steps (243.293s) bfsSolver
                    uint64_t solhash = gbl::currentGame.getHash(); HashVVV(gbl::currentGame.currentState, solhash);
                    SolveInformation info;
                    synchronized(solver::solutionMutex) {
                        std::atomic_thread_fence(std::memory_order_seq_cst);
                        if(solver::solutionDP.count(solhash) != 0) {
                            info = solver::solutionDP.at(solhash);
                        }
                        std::atomic_thread_fence(std::memory_order_seq_cst);
                    }
                    
                    if(info.success == 1) {
                        cout << "Found solution in " << info.statesExploredAStar <<" steps ("<< info.timeAStar/(1000.*1000) <<"s): ";
                        for(int i=0;i<info.solutionPath.size();++i) {
                            string moveName = info.solutionPath[i] == UP_MOVE ? "UP" : info.solutionPath[i] == DOWN_MOVE ? "DOWN" : info.solutionPath[i] == LEFT_MOVE ? "LEFT" : info.solutionPath[i] == RIGHT_MOVE ? "RIGHT" : "ACTION";
                            cout << moveName << (i+1 < info.solutionPath.size() ? "," : "");
                        }
                        cout << endl;
                        
                        for(short key : info.solutionPath) {
                            if(key == UP_MOVE) keyQueue.push({KEY_UP,100});
                            else if(key == DOWN_MOVE) keyQueue.push({KEY_DOWN,100});
                            else if(key == LEFT_MOVE) keyQueue.push({KEY_LEFT,100});
                            else if(key == RIGHT_MOVE) keyQueue.push({KEY_RIGHT,100});
                            else if(key == ACTION_MOVE) keyQueue.push({KEY_ACTION,100});
                            else {
                                DEB("This should not happen...");
                                exit(0);
                            }
                        }
                    } else if(info.success == 2) {
                        cout << "Timeout when searching for a solution." << endl;
                    } else if(info.success == 0) {
                        cout << "No solution possible." << endl;
                    }
                    
                }
                break;
            case KEY_PRINT:
                cout << "===================" << endl;
                cout << "Printing IDE String" << endl;
                cout << "===================" << endl;
                for(int i=0; i<editor::ideString.size();++i) {
                    cout << editor::ideString[i] << endl;
                }
                
                cout << "===============================" << endl;
                cout << "Printing IDE String (formatted)" << endl;
                cout << "===============================" << endl;
                cout << "{";
                for(int i=0; i<editor::ideString.size();++i) {
                    cout << "\"" << editor::ideString[i] << (i+1 != editor::ideString.size() ? "\"," : "\"") << endl;
                }
                cout << "}";
                cout << "EXPORT !!! " << endl;
                exportRecordToFile(gbl::record, "storerecord");
                break;
                
            case KEY_IMPORT:
                cout << "IMPORT !!! " << endl;
                //DISABLED FOR USERS
                importRecordFromFile(gbl::record, "storerecord");
                break;
            
            case KEY_GENERATE:
                if(gbl::mode == MODE_LEVEL_EDITOR || gbl::mode == MODE_EXPLOITATION || gbl::mode == MODE_INSPIRATION) {
                    editor::showGenerate = 1;
                    stopGenerating();
                    if(editor::successes.first && editor::successes.second) startGenerating();
                }

            default:;DEB("Unhandled key " + to_string(key));
        }
    }
    //std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<std::endl;
}

