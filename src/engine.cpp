#include "engine.h"

#include "game.h"
#include "global.h"
#include "logError.h"
#include "rules.h"
//This is supposed to be the code that runs as fast as possible as it is also used by the generator.


static vvvc generateMoveField(const short & moveDir, const vvvs & currentState, const Game & game) {
    assert((size_t)game.layerCount <= 128 && (size_t)game.currentLevelHeight <= 512 && (size_t)game.currentLevelWidth <= 512);
    vvvc nextMoveState = vvvc (game.layerCount, vector<vector<short> >(game.currentLevelHeight, vector<short>(game.currentLevelWidth)));
    
    for(int l=0;l<game.layerCount;++l) {
        for(int y=0;y<game.currentLevelHeight;++y) {
            for(int x=0;x<game.currentLevelWidth;++x) {
                if(currentState[l][y][x] == 0)
                    nextMoveState[l][y][x] = RE_MOVE;
                else
                    nextMoveState[l][y][x] = STATIONARY_MOVE;
            }
        }
    }
    
    for(int y=0;y<game.currentLevelHeight;++y) {
        for(int x=0;x<game.currentLevelWidth;++x) {
            for(int p=0;p<game.playerIndices.size();++p) {
                
                if(currentState[game.playerIndices[p].second][y][x] == game.playerIndices[p].first) {
                    nextMoveState[game.playerIndices[p].second][y][x] = moveDir;
                }
            }
        }
    }
    
    return nextMoveState;
}

static inline void moveCollisions(vvvs & currentState, vvvc & currentMoveState, const Game & game) {
    //Do all move collisions:
    for(int l=0;l<game.layerCount;++l) {
        
        /*
        cout << "DEBUG MOVE MAP" << endl;
        for(int y=0;y<game.currentLevelHeight;++y) {
            for(int x=0;x<game.currentLevelWidth;++x) {
                if(currentMoveState[l][y][x] == STATIONARY_MOVE) cout << "S";
                else if(currentMoveState[l][y][x] == RE_MOVE) cout << ".";
                else if(currentMoveState[l][y][x] == ACTION_MOVE) cout << "a";
                else if(currentMoveState[l][y][x] == LEFT_MOVE) cout << "<";
                else if(currentMoveState[l][y][x] == RIGHT_MOVE) cout << ">";
                else if(currentMoveState[l][y][x] == UP_MOVE) cout << "^";
                else if(currentMoveState[l][y][x] == DOWN_MOVE) cout << "v";
                else cout << "?";
            }
            cout << endl;
        }
        cout << endl;*/
        
        for(int y=0;y<game.currentLevelHeight;++y) {
            for(int x=0;x<game.currentLevelWidth;++x) {
                if(currentMoveState[l][y][x] == ACTION_MOVE) currentMoveState[l][y][x] = STATIONARY_MOVE;
                //assert(currentMoveState[l][y][x] == UP_MOVE || currentMoveState[l][y][x] == DOWN_MOVE || currentMoveState[l][y][x] == LEFT_MOVE || currentMoveState[l][y][x] == RIGHT_MOVE || currentMoveState[l][y][x] == STATIONARY_MOVE || currentMoveState[l][y][x] == RE_MOVE);
                
                if(currentMoveState[l][y][x] == STATIONARY_MOVE || currentMoveState[l][y][x] == RE_MOVE) continue;
                stack<pair<short,short> > s;
                int cy = y, cx = x;
                
                for(;;) {
                    short c = currentMoveState[l][cy][cx];
                    int ny = cy+(c == UP_MOVE ? -1 : c == DOWN_MOVE ? 1 : 0);
                    int nx = cx+(c == LEFT_MOVE ? -1 : c == RIGHT_MOVE ? 1 : 0);
                    char oppositeC = c==UP_MOVE ? DOWN_MOVE : c==DOWN_MOVE ? UP_MOVE : c==LEFT_MOVE ? RIGHT_MOVE : LEFT_MOVE;
                    
                    if(ny<0||nx<0||ny>=game.currentLevelHeight||nx>=game.currentLevelWidth||currentMoveState[l][ny][nx] == STATIONARY_MOVE||currentMoveState[l][ny][nx]==oppositeC) { // > x => x x and > < => x x and > edge => x edge
                        
                        currentMoveState[l][cy][cx] = STATIONARY_MOVE;
                        while(!s.empty()) {
                            currentMoveState[l][s.top().first][s.top().second] = STATIONARY_MOVE;
                            s.pop();
                        }
                        break;
                    } else if(currentMoveState[l][ny][nx] == RE_MOVE) { //> . => . x
                        currentState[l][ny][nx] = currentState[l][cy][cx];
                        currentMoveState[l][ny][nx] = STATIONARY_MOVE;
                        while(!s.empty()) {
                            currentState[l][cy][cx] = currentState[l][s.top().first][s.top().second];
                            currentMoveState[l][cy][cx] = STATIONARY_MOVE;
                            cy = s.top().first, cx = s.top().second;
                            s.pop();
                        }
                        currentState[l][cy][cx] = 0;
                        currentMoveState[l][cy][cx] = RE_MOVE;
                        break;
                    } else {
                        s.push({cy,cx});
                        cx = nx;
                        cy = ny;
                    }
                }
            }
        }
        
        
        for(int y=0;y<game.currentLevelHeight;++y) {
            for(int x=0;x<game.currentLevelWidth;++x) {
                if(currentState[l][y][x] == 0) assert(currentMoveState[l][y][x] == RE_MOVE);
                else assert(currentMoveState[l][y][x] == STATIONARY_MOVE);
            }
        }
    }
}

static inline bool matchesLayer(const Rule & r, const int s, const int x, const int y, const bool isrightrule, const vvvs & currentState, const vvvc & currentMoveState) {
    for(int t=0;t<r.lhsObjects[s].size();++t) {
        int cx = isrightrule ? x+t : x;
        int cy = isrightrule ? y : y+t; //if not right rule, it is a down rule
        for(int u=0;u<r.lhsObjects[s][t].size();++u) {
            short layer = r.lhsLayers[s][t][u];
            if(currentState[layer][cy][cx] == r.lhsObjects[s][t][u]) {
                //r.lhsDirs[s][t][u] !=
                if(r.lhsDirs[s][t][u] == RE_MOVE || (r.lhsDirs[s][t][u] < RE_MOVE && (currentMoveState[layer][cy][cx] & r.lhsDirs[s][t][u]) == 0 && !(r.lhsDirs[s][t][u] == STATIONARY_MOVE && currentMoveState[layer][cy][cx] == STATIONARY_MOVE) ) ) return false;
                
                //make sure the following example works: https://www.puzzlescript.net/editor.html?hack=7f58161c8759c377a9af4280e9e0dfad
                
                //if(r.lhsDirs[s][t][u] <= RE_MOVE && (currentMoveState[layer][cy][cx] & r.lhsDirs[s][t][u] & 0b11111) == 0) return false;

                //definitely wrong:
                //if(r.lhsDirs[s][t][u] <= RE_MOVE && (currentMoveState[layer][cy][cx] & r.lhsDirs[s][t][u]) == 0) return false;
            } else {
                if(r.lhsDirs[s][t][u] != RE_MOVE) return false;
            }
        }
    }
    return true;
}


//returns -2 if it doesn't match, else returns 0 (or the ellipsis offset in [-1,..+oo))
static inline int matchesLayerEllipsis(const Rule & r, const int s, const int x, const int y, const bool isrightrule, const vvvs & currentState, const vvvc & currentMoveState) {
    int ellipsisoffset = 0;
    bool ellipsis = false;
    for(int t=0;t<r.lhsObjects[s].size();++t) {
        if(r.lhsTypes[s][t] == TYPE_ELLIPSIS) {
            ellipsis = true;
            ellipsisoffset = -1; //since we use t+=1
            t+=1;
        }
        do {
            int cx = isrightrule ? x+t+ellipsisoffset : x;
            int cy = isrightrule ? y : y+t+ellipsisoffset; //if not right rule, it is a down rule
            if(cx >= currentState[0][0].size() || cy >= currentState[0].size()) return -2;
            
            for(int u=0;u<r.lhsObjects[s][t].size();++u) {
                short layer = r.lhsLayers[s][t][u];
                if(currentState[layer][cy][cx] == r.lhsObjects[s][t][u]) {
                    //r.lhsDirs[s][t][u] !=
                    if(r.lhsDirs[s][t][u] == RE_MOVE || (r.lhsDirs[s][t][u] < RE_MOVE &&    (currentMoveState[layer][cy][cx] & r.lhsDirs[s][t][u]) == 0 && !(r.lhsDirs[s][t][u]     == STATIONARY_MOVE && currentMoveState[layer][cy][cx] == STATIONARY_MOVE) ) ) {
                        if(!ellipsis) return -2;
                    }
                    else ellipsis = false; // found match
                    //make sure the following example works:    https://www.puzzlescript.net/editor.html?hack=7f58161c8759c377a9af4280e9e0dfad
                } else {
                    if(r.lhsDirs[s][t][u] != RE_MOVE) if(!ellipsis) return -2;
                }
            }
            if(ellipsis) ellipsisoffset += 1;
        } while(ellipsis);
    }
    return ellipsisoffset;
}

//unused, no ellipsis
static inline void replacesLayer(const Rule & r, const Game & game, const int s, const int x, const int y, const bool isrightrule, vvvs & currentState, vvvc & currentMoveState) {
    for(int t=0;t<r.rhsObjects[s].size();++t) {
        int cx = isrightrule ? x+t : x;
        int cy = isrightrule ? y : y+t; //if not right rule, it is a down rule
        //first remove lhs blocks
        int sl = r.lhsObjects[s][t].size(), sr = r.rhsObjects[s][t].size();
        for(int u=0;u<sl;++u) {
            if(r.lhsDirs[s][t][u] != RE_MOVE) {
                short layer = r.lhsLayers[s][t][u];
                //cout << "removing " << game.objPrimaryName[currentState[layer][cy][cx]] << " at " << cy << " " << cx << " on layer " << layer << endl;
                currentState[layer][cy][cx] = 0;
                currentMoveState[layer][cy][cx] = RE_MOVE;
            }
        }
        
        for(int u=0;u<sr;++u) {
            short layer = r.rhsLayers[s][t][u];
            //then update accordingly
            if(r.rhsDirs[s][t][u] == RE_MOVE) { //no
                if(currentState[layer][cy][cx] == r.rhsObjects[s][t][u]) {
                    currentState[layer][cy][cx] = 0;
                    currentMoveState[layer][cy][cx] = RE_MOVE;
                }
            } else {
                //set block
                currentState[layer][cy][cx] = r.rhsObjects[s][t][u];
                currentMoveState[layer][cy][cx] = r.rhsDirs[s][t][u];
            }
            
        }
    }
}


static inline void replacesLayerEllipsis(const Rule & r, const Game & game, const int s, const int x, const int y, const bool isrightrule, vvvs & currentState, vvvc & currentMoveState, int ellipsisoffset) {
    bool ellipsis = false;
    for(int t=0;t<r.rhsObjects[s].size();++t) {
        if(r.lhsTypes[s][t] == TYPE_ELLIPSIS) {
            ellipsis = true;
            t += 1;
        }
        int cx = isrightrule ? x+t+(ellipsis?ellipsisoffset:0) : x;
        int cy = isrightrule ? y : y+t+(ellipsis?ellipsisoffset:0); //if not right rule, it is a down rule
        //first remove lhs blocks
        int sl = r.lhsObjects[s][t].size(), sr = r.rhsObjects[s][t].size();
        for(int u=0;u<sl;++u) {
            if(r.lhsDirs[s][t][u] != RE_MOVE) {
                short layer = r.lhsLayers[s][t][u];
                //cout << "removing " << game.objPrimaryName[currentState[layer][cy][cx]] << " at " << cy << " " << cx << " on layer " << layer << endl;
                currentState[layer][cy][cx] = 0;
                currentMoveState[layer][cy][cx] = RE_MOVE;
            }
        }
        
        for(int u=0;u<sr;++u) {
            short layer = r.rhsLayers[s][t][u];
            //then update accordingly
            if(r.rhsDirs[s][t][u] == RE_MOVE) { //no
                if(currentState[layer][cy][cx] == r.rhsObjects[s][t][u]) {
                    currentState[layer][cy][cx] = 0;
                    currentMoveState[layer][cy][cx] = RE_MOVE;
                }
            } else {
                //set block
                currentState[layer][cy][cx] = r.rhsObjects[s][t][u];
                currentMoveState[layer][cy][cx] = r.rhsDirs[s][t][u];
            }
            
        }
    }
}

//unused
static inline vector<vector<pair<int,int> > > findMatches(const Rule & r, const vvvs & currentState, const vvvc & currentMoveState, bool & again, const Game & game) {
    vector<vector<pair<int,int> > > matches (r.lhsObjects.size());
    if(r.direction == DIR_RIGHT) {
        for(size_t s=0;s<r.lhsObjects.size();++s) {
            for(int y=0;y<game.currentLevelHeight;++y) {
                for(int x=0;x<game.currentLevelWidth;++x) {
                    if(x + r.lhsObjects[s].size() > game.currentLevelWidth) break;
                    bool match = matchesLayer(r,s,x,y,true,currentState,currentMoveState);
                    if(match) matches[s].push_back({y,x});
                }
            }
        }
    }
    else { //r.direction == DIR_DOWN
        for(size_t s=0;s<r.lhsObjects.size();++s) {
            for(int x=0;x<game.currentLevelWidth;++x) {
                for(int y=0;y<game.currentLevelHeight;++y) {
                    if(y + r.lhsObjects[s].size() > game.currentLevelHeight) break;
                    bool match = matchesLayer(r,s,x,y,false,currentState,currentMoveState);
                    if(match) matches[s].push_back({y,x});
                }
            }
        }
    }
    return matches;
}

//unused, no ellipsis
static inline bool advanceMatchRight(pair<int,int> & pointer, const int & s, const Rule & r, const vvvs & currentState, const vvvc & currentMoveState, const Game & game) {
    for(int y=pointer.second;y<game.currentLevelHeight;++y) {
        for(int x=(y==pointer.second?pointer.first:0);x<game.currentLevelWidth;++x) {
            if(x + r.lhsObjects[s].size() > game.currentLevelWidth) break;
            bool match = matchesLayer(r,s,x,y,true,currentState,currentMoveState);
            if(match) {
                pointer.first = x;
                pointer.second = y;
                return true;
            }
        }
        //pointer.first = 0;
    }
    return false;
}
//unused, no ellipsis
static inline bool advanceMatchDown(pair<int,int> & pointer, const int & s, const Rule & r, const vvvs & currentState, const vvvc & currentMoveState, const Game & game) {
    for(int x=pointer.first;x<game.currentLevelWidth;++x) {
        for(int y=(x==pointer.first?pointer.second:0);y<game.currentLevelHeight;++y) {
            if(y + r.lhsObjects[s].size() > game.currentLevelHeight) break;
            bool match = matchesLayer(r,s,x,y,false,currentState,currentMoveState);
            if(match) {
                pointer.first = x;
                pointer.second = y;
                return true;
            }
        }
    }
    return false;
}

static inline int advanceMatchRightEllipsis(pair<int,int> & pointer, const int & s, const Rule & r, const vvvs & currentState, const vvvc & currentMoveState, const Game & game) {
    for(int y=pointer.second;y<game.currentLevelHeight;++y) {
        for(int x=(y==pointer.second?pointer.first:0);x<game.currentLevelWidth;++x) {
            if(x + r.lhsObjects[s].size() > game.currentLevelWidth) break;
            int ret = matchesLayerEllipsis(r,s,x,y,true,currentState,currentMoveState);
            if(ret >= -1) {
                pointer.first = x;
                pointer.second = y;
                return ret;
            }
        }
        //pointer.first = 0;
    }
    return -2;
}

static inline int advanceMatchDownEllipsis(pair<int,int> & pointer, const int & s, const Rule & r, const vvvs & currentState, const vvvc & currentMoveState, const Game & game) {
    for(int x=pointer.first;x<game.currentLevelWidth;++x) {
        for(int y=(x==pointer.first?pointer.second:0);y<game.currentLevelHeight;++y) {
            if(y + r.lhsObjects[s].size() > game.currentLevelHeight) break;
            int ret = matchesLayerEllipsis(r,s,x,y,false,currentState,currentMoveState);
            if(ret >= -1) {
                pointer.first = x;
                pointer.second = y;
                return ret;
            }
        }
    }
    return -2;
}


//computed 41000 steps in 58475ms with current best cost: 2
//computed 45000 steps in 58578ms with current best cost: 2


//returns true if the method executeRuleDirectly had a match
static inline bool executeRuleNonOption(const Rule & r, vvvs & currentState, vvvc & currentMoveState, const Game & game) {
    assert(!r.choose);
    vector<pair<int,int> > pointers (r.lhsObjects.size(), make_pair(0,0));
    
    bool hasMatch = false;
    
    if(r.direction == DIR_RIGHT) {
        for(;;) {
            for(size_t s=0;s<r.lhsObjects.size();++s) {
                //bool ellipsis = false;
                //for(size_t j=0;j<r.lhsObjects[s].size();++j) {}
                int ret = advanceMatchRightEllipsis(pointers[s], s, r, currentState, currentMoveState, game);
                if(ret >= -1) { //there was a match = 0, if ret>0 that is the ellipsisoffset.
                    if(s+1==r.lhsObjects.size()) {
                        for(int sr=0;sr<r.rhsObjects.size();++sr)
                            replacesLayerEllipsis(r, game, sr, pointers[sr].first, pointers[sr].second, true, currentState, currentMoveState, ret);
                        
                        pointers.back().first += 1;
                        hasMatch = true;
                    }
                }
                else return hasMatch;
            }
            
        }
    } else { //r.direction == DIR_DOWN
        for(;;) {
            for(size_t s=0;s<r.lhsObjects.size();++s) {
                int ret = advanceMatchDownEllipsis(pointers[s], s, r, currentState, currentMoveState, game);
                if(ret >= -1) {
                    if(s+1 == r.lhsObjects.size()) {
                        for(size_t sr=0;sr<r.rhsObjects.size();++sr)
                            replacesLayerEllipsis(r, game, sr, pointers[sr].first, pointers[sr].second, false, currentState, currentMoveState, ret);
                        pointers.back().second += 1;
                        hasMatch = true;
                    }
                }
                else return hasMatch;
                /*
                if(advanceMatchDown(pointers[s], s, r, currentState, currentMoveState, game)) {
                    if(s+1 == r.lhsObjects.size()) {
                        for(size_t sr=0;sr<r.rhsObjects.size();++sr)
                            replacesLayer(r, game, sr, pointers[sr].first, pointers[sr].second, false, currentState, currentMoveState);
                        pointers.back().second += 1;
                        hasMatch = true;
                    }
                }
                else return hasMatch;
                 */
            }
        }
    }
}


//returns true if the method executeRuleDirectly had a match
static inline pair<vector<vvvs>, vector<vvvs> > executeRuleSingleMatch(const Rule & r, const vector<vvvs> & currentStates, const vector<vvvc> & currentMoveStates, int maxStates, const Game & game, const vector<vector<bool> > & modifyTable) {
    
    //if( rand()/RAND_MAX > r.optionProb) return make_pair(outStates, outMoveStates);;
    vector<vector<vector<pair<int,int> > > > matchLocations (currentStates.size(), vector<vector<pair<int,int> > > (r.lhsObjects.size()));
    
    if(r.direction == DIR_RIGHT) {
        for(size_t q=0;q<currentStates.size();++q) {
            for(size_t s=0;s<r.lhsObjects.size();++s) {
                pair<int,int> pointer = make_pair(0,0);
                while(advanceMatchRight(pointer,s,r,currentStates[q],currentMoveStates[q],game)) {
                    bool canBeModified = true;
                    for(int i=pointer.first;i<pointer.first+r.lhsObjects[s].size();++i) {
                        if(!modifyTable[pointer.second][i]) {
                            canBeModified = false;
                            break;
                        }
                    }
                    if(canBeModified)
                        matchLocations[q][s].push_back(pointer);
                    pointer.first += 1;
                }
            }
        }
    } else { //r.direction == DIR_DOWN
        for(size_t q=0;q<currentStates.size();++q) {
            for(size_t s=0;s<r.lhsObjects.size();++s) {
                pair<int,int> pointer = make_pair(0,0);
                while(advanceMatchDown(pointer,s,r,currentStates[q],currentMoveStates[q],game)) {
                    bool canBeModified = true;
                    for(size_t i=pointer.second;i<pointer.second+r.lhsObjects[s].size();++i) {
                        if(!modifyTable[i][pointer.first]) {
                            canBeModified = false;
                            break;
                        }
                    }
                    if(canBeModified)
                        matchLocations[q][s].push_back(pointer);
                    pointer.second += 1;
                }
            }
        }
    }
    
    assert(currentStates.size() <= maxStates);
    //every of the currentState gets exactly one
    vector<vvvs> outStates(maxStates), outMoveStates(maxStates);
    
    for(int i=0;i<maxStates;++i) {
        int pickfrom = i % currentStates.size();
        
        int totalAmount = r.rhsObjects.size()==0 ? 0 : 1;
        for(int sr=0;sr<r.rhsObjects.size();++sr) {
            totalAmount *= matchLocations[pickfrom][sr].size();
        }
        
        outStates[i] = currentStates[pickfrom];
        outMoveStates[i] = currentMoveStates[pickfrom];
        if(totalAmount == 0) continue;
        int chooseOption = rand() % (totalAmount);
        //if(chooseOption < totalAmount) { //if chooseOption == totalAmount do nothing
        //replace according to random selection
        for(int sr=0; sr<r.rhsObjects.size();++sr) {
            pair<int,int> pointer = matchLocations[pickfrom][sr][chooseOption % matchLocations[pickfrom][sr].size()];
            replacesLayer(r, game, sr, pointer.first, pointer.second, r.direction == DIR_RIGHT, outStates[i], outMoveStates[i]);
            chooseOption /= matchLocations[pickfrom][sr].size();
        }
    }
    
    return make_pair(outStates, outMoveStates);
}


static void engineStep(vvvs & currentState, vvvc & currentMoveState, const Game & game) {
    vvvc oldCurrentState = currentState;
    vvvc oldCurrentMoveState = currentMoveState;
    
    bool again = false;
    bool inLatePhase = false;
    vvvc groupState, groupMoveState;
    int loopedRule = -1, loopCount = 0;
    bool singleRule = true;
    for(int ri=0;ri<game.rules.size();++ri) {
        const Rule & r = game.rules[ri];
        //printRule(cout, r, game);
        
        //figure out if the next rule is
        
        if(r.groupNumber == ri && ri+1!= game.rules.size() && game.rules[ri].groupNumber == ri) {
            groupState = currentState; // deep copy the current state.
            groupMoveState = currentMoveState;
        }
        
        if(r.late && !inLatePhase) {
            moveCollisions(currentState, currentMoveState, game);
            inLatePhase = true;
        }
        
        //cout << "Exec" << endl;
        //printRule(cout, r, game);
        //cout << "try " << inLatePhase << ": ";
        //printRule(cout, r, game);
        
        bool hadMatch = executeRuleNonOption(r, currentState, currentMoveState, game);
        if(hadMatch) { //execute additional commands after executing the rule
            //cout << "OK!" << endl;
            //printRule(cout, r, game);
            for(Commands cmd : r.commands) {
                if(cmd == CMD_CANCEL) {
                    //cout << "CANCEL CANCEL" << endl;
                    currentState = oldCurrentState;
                    currentMoveState = oldCurrentMoveState;
                    return;
                } else if(cmd == CMD_AGAIN) {
                    again = true;
                } else if(cmd == CMD_WIN || cmd == CMD_RESTART || cmd == CMD_CHECKPOINT) {}
                else { //sfx
                    
                }
            }
        }
        

        if(r.groupNumber != ri && (ri+1 == game.rules.size() || r.groupNumber != game.rules[ri+1].groupNumber) && (groupState != currentState || groupMoveState != currentMoveState)) {
            ri = r.groupNumber-1; // restart the iteration
            if(loopedRule != ri+1) {
                loopedRule = ri+1;
                loopCount = 0;
            }
            loopCount++;
            if(loopCount > 200) {
                cerr << "Warning: Rule group " << r.groupNumber << " has been looped " << loopCount << " times!" << endl;
                currentState = oldCurrentState;
                currentMoveState = oldCurrentMoveState;
                return;
            }
            continue;
        }
    }
    
    if(!inLatePhase) {
        moveCollisions(currentState, currentMoveState, game);
        inLatePhase = true;
    }
    
    if(again && (oldCurrentState != currentState || oldCurrentMoveState != currentMoveState)) {
        engineStep(currentState, currentMoveState, game);
    }
}

//currentStates contains all generated levels. currentStates[from]
//generates all possible states coming after prevState (can be bounded by a certain number if there are too many, 1 if only one should be chosen).
//it might return a vector with less than maxStates states if there is no matching.

vector<vvvs> generateStep(const vvvs & prevState, int maxStates, const Game & game, const vector<vector<bool> > & modifyTable) {
    vector<vvvs> currentStates (maxStates, prevState);
    vvvc emptyMoveState = generateMoveField(STATIONARY_MOVE, prevState, game);
    vector<vvvc> currentMoveStates (maxStates, emptyMoveState);
    
    bool again = false;
    bool inLatePhase = false;
    vector<vvvc> groupStates, groupMoveState;
    int loopedRule = -1, loopCount = 0;
    bool singleRule = true;
    
    for(int ri=0;ri<game.generatorRules.size();++ri) {
        const Rule & r = game.generatorRules[ri];
        //printRule(cout, r, game);
        
        //figure out if the next rule is
        
        if(r.groupNumber == ri && ri != game.rules.size() && game.generatorRules[ri].groupNumber == ri) {
            groupStates = currentStates; // deep copy the current state.
            groupMoveState = currentMoveStates;
        }
        
        if(r.late && !inLatePhase) {
            for(int i=0;i<currentStates.size(); ++i)
                moveCollisions(currentStates[i], currentMoveStates[i], game);
            inLatePhase = true;
        }
        
        if(r.choose) {
            //execute r.chooseCount rules of rules with r.groupNumber
            int untilri = ri;
            for(;untilri < game.generatorRules.size() && game.generatorRules[untilri].groupNumber == r.groupNumber; untilri++) {}
            
            vector<int> chosenRules(untilri-ri);
            for(int i=0;i<chosenRules.size();++i) chosenRules[i]=ri+i;
            for(int i=0;i<r.choose;++i) {
                int selectedrule = ri+(rand()%(untilri-ri)); //choose randomly from the rules with equal probability (can be skewed by the option keyword)
                
                if( rand()/RAND_MAX <= r.optionProb) { //check whether it will be executed or not
                    auto p = executeRuleSingleMatch(game.generatorRules[selectedrule], currentStates, currentMoveStates, maxStates, game, modifyTable);
                    currentStates = p.first;
                    currentMoveStates = p.second;
                }
            }
            ri = untilri-1;

        } else {
            for(int i=0;i<currentStates.size();++i) {
                executeRuleNonOption(r, currentStates[i], currentMoveStates[i], game);
            }
        }
        
        /*
        if(!r.option) {
            for(int i=0;i<currentStates.size();++i) {
                executeRuleNonOption(r, currentStates[i], currentMoveStates[i], game);
            }
        } else {
            auto p = executeRuleOption(r, currentStates, currentMoveStates, maxStates, game, modifyTable);
            currentStates = p.first;
            currentMoveStates = p.second;
        }*/
        
        
        //In generate mode groups get handled differently.
        if(!r.choose && r.groupNumber != ri && (ri+1 == game.rules.size() || r.groupNumber != game.rules[ri+1].groupNumber) && (groupStates != currentStates || groupMoveState != currentMoveStates)) {
            ri = r.groupNumber-1; // restart the iteration
            if(loopedRule != ri+1) {
                loopedRule = ri+1;
                loopCount = 0;
            }
            loopCount++;
            if(loopCount > 20) {
                cout << "Warning: Rule group " << r.groupNumber << " has been looped " << loopCount << " times!" << endl;
            }
            continue;
        }
    }
    
    static int counter = 0;
    if(!inLatePhase) {
        for(int i=0;i<currentStates.size(); ++i) {
            moveCollisions(currentStates[i], currentMoveStates[i], game);
        }
        inLatePhase = true;
    }
    return currentStates;
}


//returns 0 if win condition is fulfilled, otherwise returns an estimated cost of the distance to the winning condition
static bool checkWinCondition(vvvs & currentState, const Game & game) {
    
    for(const auto & w : game.winConditionsNo)
        for(int y=0;y<game.currentLevelHeight;++y)
            for(int x=0;x<game.currentLevelWidth;++x)
                if(currentState[w.second][y][x] == w.first)
                    return false;
    
    for(const auto & wv : game.winConditionsSome) {
        for(const auto & w : wv) {
            bool success = false;
            for(int y=0;y<game.currentLevelHeight;++y) {
                for(int x=0;x<game.currentLevelWidth;++x) {
                    if(currentState[w.second][y][x] == w.first) {
                        x=game.currentLevelWidth;
                        y=game.currentLevelHeight;
                        success = true;
                    }
                }
            }
            if(!success) return false;
        }
    }
    
    //faster way of checking No X on Y:
    for(int i = 0; i<game.winConditionsNoXOnYLHS.size(); ++i) {
        for(int l=0; l<game.winConditionsNoXOnYLHS[i].size();++l) {
            int lindex = game.winConditionsNoXOnYLHS[i][l].first;
            int llayer = game.winConditionsNoXOnYLHS[i][l].second;
            for(int y=0;y<game.currentLevelHeight;++y) {
                for(int x=0;x<game.currentLevelWidth;++x) {
                    if(currentState[llayer][y][x] == lindex) {
                        for(int r=0; r<game.winConditionsNoXOnYRHS[i].size(); ++r) { //for every LHS check that it is not on a RHS.
                            int rindex = game.winConditionsNoXOnYRHS[i][r].first;
                            int rlayer = game.winConditionsNoXOnYRHS[i][r].second;
                            
                            if(currentState[llayer][y][x] == lindex) {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
    
    for(int i = 0; i < game.winConditionsSomeXOnYLHS.size(); ++i) {
        bool success = false;
        for(int l=0; l<game.winConditionsSomeXOnYLHS[i].size(); ++l) {
            int lindex = game.winConditionsSomeXOnYLHS[i][l].first;
            int llayer = game.winConditionsSomeXOnYLHS[i][l].second;
            
            for(int y=0;y<game.currentLevelHeight;++y) {
                for(int x=0;x<game.currentLevelWidth;++x) {
                    if(currentState[llayer][y][x] == lindex) {
                        for(int r=0; r<game.winConditionsSomeXOnYRHS[i].size();++r) {
                            int rindex = game.winConditionsSomeXOnYRHS[i][r].first;
                            int rlayer = game.winConditionsSomeXOnYRHS[i][r].second;
                            if(currentState[rlayer][y][x] == rindex) {
                                success = true;
                                x=game.currentLevelWidth;
                                y=game.currentLevelHeight;
                                r = game.winConditionsSomeXOnYRHS[i].size();
                                l = game.winConditionsSomeXOnYLHS[i].size();
                            }
                        }
                    }
                }
            }
        }
        if(!success) return false;
    }
    
    
    
    for(int i = 0; i < game.winConditionsAllXOnYLHS.size(); ++i) {
        for(int l=0; l<game.winConditionsAllXOnYLHS[i].size(); ++l) {
            int lindex = game.winConditionsAllXOnYLHS[i][l].first;
            int llayer = game.winConditionsAllXOnYLHS[i][l].second;
            
            for(int y=0;y<game.currentLevelHeight;++y) {
                for(int x=0;x<game.currentLevelWidth;++x) {
                    if(currentState[llayer][y][x] == lindex) {
                        bool success = false;
                        for(int r=0; r<game.winConditionsAllXOnYRHS[i].size(); ++r) {
                            int rindex = game.winConditionsAllXOnYRHS[i][r].first;
                            int rlayer = game.winConditionsAllXOnYRHS[i][r].second;
                            if(currentState[rlayer][y][x] == rindex) success = true;
                        }
                        if(!success) return false;
                    }
                }
            }
        }
    }
    
    return true;
}


void moveAndChangeField(const short & moveDir, vvvs & currentState, const Game & game) {
    vvvc moveField = generateMoveField(moveDir, currentState, game);
    engineStep(currentState, moveField, game);
}

bool moveAndChangeFieldAndReturnWinningCondition(const short & moveDir, vvvs & currentState, const Game & game) {
    vvvc moveField = generateMoveField(moveDir, currentState, game);
    engineStep(currentState, moveField, game);
    return checkWinCondition(currentState, game);
}



// This will handle all unnecessary things about movements that are not needed when brute forcing levels.
bool move(const short & moveDir, Game & game) {
    vvvs oldState = game.currentState;
    
    bool winning = moveAndChangeFieldAndReturnWinningCondition(moveDir, game.currentState, game);
    
    if(oldState != game.currentState) { //add movement towards undo map.
        game.undoStates.push_back({ oldState, moveDir });
    }
    
    return winning;
}

void undo(Game & game) {
    cout << "UNDO" << endl;
    if(game.undoStates.size() > 0) {
        game.currentState = game.undoStates.back().first;
        game.undoStates.pop_back();
    }
}

void restart(Game & game) {
    game.undoStates.push_back({game.currentState,(short)0x0101010}); //restart key = 0x0101010
    game.currentState = game.levels[game.currentLevelIndex];
    move(STATIONARY_MOVE, game);
    if(game.undoStates.back().second != (short)0x0101010) game.undoStates.pop_back(); //stationary move shouldn't be undoable
    
    //don't clear the undoStates, so you can undo the restart
}
