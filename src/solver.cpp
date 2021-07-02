#include "solver.h"

//#include "ofxMSAmcts.h"

#include "engine.h"
#include "game.h"
#include "global.h"

#include <unordered_set>

namespace solver {
    recursive_mutex solutionMutex;
    //map<pair<uint64_t,vvvs>, SolveInformation> solutionDP;
    unordered_map<uint64_t, SolveInformation> solutionDP;
}

SolveInformation mergeSolveInformation(const SolveInformation & a, const SolveInformation & b) {
    SolveInformation i = a;
    if(i.solutionPath.size()  == 0 || (b.solutionPath.size() != 0 && b.solutionPath.size() < i.solutionPath.size()))
        i.solutionPath = b.solutionPath;
    if(i.shortestSolutionPath.size() == 0 && b.shortestSolutionPath.size() != 0)
        i.shortestSolutionPath = b.shortestSolutionPath;
    if(i.success == 2)
        i.success = b.success;
    if(i.success == 0 && b.success == 1)
        i.success = b.success;
    //assert(!(a.success == 0 && b.success == 1) && !(a.success == 1 && b.success == 0));

    i.layersWithoutSolution = MAX(i.layersWithoutSolution, b.layersWithoutSolution);
    
    i.statesExploredBFS = MAX(i.statesExploredBFS, b.statesExploredBFS);
    i.statesExploredHeuristic = MAX(i.statesExploredHeuristic, b.statesExploredHeuristic);
    i.statesExploredAStar = MAX(i.statesExploredAStar, b.statesExploredAStar);
    
    i.timeBFS = MAX(i.timeBFS, b.timeBFS);
    i.timeHeuristic = MAX(i.timeHeuristic, b.timeHeuristic);
    i.timeAStar = MAX(i.timeAStar, b.timeAStar);
    
    return i;
}

void addAdditionalToSolvePath(vvvs state, deque<short> moves, const Game & game) {
    SolveInformation info;
    for(;;) {
        uint64_t solhash = game.getHash();
        HashVVV(state, solhash);
        SolveInformation info;
        info.success = 1;
        info.solutionPath = moves;
        synchronized(solver::solutionMutex) {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            if(solver::solutionDP.count(solhash) != 0) {
                // cout << "READ2 " << solver::solutionDP.at(solhash).shortestSolutionPath.size() << endl;
                info = mergeSolveInformation(solver::solutionDP.at(solhash), info);
                // cout << "SET2: " << info.shortestSolutionPath.size() << " " << solhash << endl;
                solver::solutionDP.at(solhash) = info;
                // cout << "RES2: " << info.shortestSolutionPath.size() << endl;
            } else {
                solver::solutionDP.insert({solhash,info});
            }
            std::atomic_thread_fence(std::memory_order_seq_cst);
        }
        if(moves.empty()) break;
        moveAndChangeField(moves.front(), state, game);
        moves.pop_front();
    }
}



SolveInformation bfsSolver(const vvvs & stateToBeSolved, const Game & game, int memoizationMaxSize, long long maxMillis,
                           const deque<short> & additionalZeroCostMoves, bool dpLookup,
                           volatile std::atomic_bool & keepSolving) {
    uint64_t solhash = game.getHash(); HashVVV(stateToBeSolved, solhash);
    
    synchronized(solver::solutionMutex) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        if(solver::solutionDP.count(solhash) != 0) {
            SolveInformation known = solver::solutionDP.at(solhash);
            if(known.success == 0) return known; //unsolvable
            if(known.shortestSolutionPath.size() != 0) return known; //BFS minimal solution found.
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    
    uint64_t chash = INITIAL_HASH;
    HashVVV(stateToBeSolved, chash);
    unordered_set<uint64_t> computed = {chash};
    
    queue<vvvs > qstates;
    qstates.push(stateToBeSolved);
    int memoizationSize = memoizationMaxSize >- 1 ? MIN(10007, memoizationMaxSize) : 10007;
    vector<pair<size_t,short> > previousMove (memoizationSize+6); //used for reconstructing the solution.

    
    chrono::steady_clock::time_point beginTime = chrono::steady_clock::now();
    chrono::steady_clock::time_point lastBFSSaveTime = chrono::steady_clock::now();
    
    bool success = false, timeout = false;
    long long totalsteps = 0;
    int prevshortestsolutionsize = 1;
    vector<short> moves = {UP_MOVE,DOWN_MOVE,LEFT_MOVE,RIGHT_MOVE,ACTION_MOVE};
    for(size_t steps = 0; !qstates.empty(); ++steps) {
        if(totalsteps >= memoizationSize) {
            memoizationSize *= 2;
            previousMove.resize(memoizationSize+6);
        }
        
        //checks for timeout or cancellation
        if(!keepSolving || ((steps < 100 || totalsteps % 100 == 0) && (maxMillis >- 1 && chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - beginTime).count() > maxMillis))) {
            success = false;
            timeout = true;
            break;
        }
        
        //update best known BFS solution every second
        if(steps % 100 == 0 && chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - lastBFSSaveTime).count() > 1000) {
            int sizeofshortest = -1;
            for(size_t index = totalsteps; index > 0; index = previousMove[index].first) {
                sizeofshortest++;
            }
            synchronized(solver::solutionMutex) { //update BFS depth
                std::atomic_thread_fence(std::memory_order_seq_cst);
                SolveInformation info;
                info.layersWithoutSolution = sizeofshortest;
                if(solver::solutionDP.count(solhash) != 0) {
                    // cout << "READ3 " << solver::solutionDP.at(solhash).shortestSolutionPath.size() << endl;
                    info = mergeSolveInformation(solver::solutionDP.at(solhash), info);
                    // cout << "SET3: " << info.shortestSolutionPath.size() << " " << solhash << endl;
                    solver::solutionDP.at(solhash) = info;
                    // cout << "RES3: " << info.shortestSolutionPath.size() << endl;
                } else {
                    solver::solutionDP.insert({solhash, info});
                }
                std::atomic_thread_fence(std::memory_order_seq_cst);
            }
            lastBFSSaveTime = chrono::steady_clock::now();
        }
        
        
        const vvvs cp = qstates.front();
        qstates.pop();
        
        random_shuffle(moves.begin(), moves.end());
        
        for(short dir : moves) {
            vvvs c = cp;
            success = moveAndChangeFieldAndReturnWinningCondition(dir, c, game);
            uint64_t chash = INITIAL_HASH;
            HashVVV(c, chash);
            if(computed.count(chash) == 0) {
                previousMove[++totalsteps] = {steps,dir};
                computed.insert(chash);
                qstates.push(c);
                if(success) break;
            }
        }
        if(success) break;
        
        if(totalsteps >= memoizationMaxSize && memoizationMaxSize >- 1) {
            timeout = true;
            success = false;
            break;
        }
    }
    
    //cout << "found after " << totalsteps << " steps in " << chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - beginTime).count() << "ms" << endl;
    
    //vvvs isfinal = qstates.back();
    //game.currentState = isfinal; // i thought this was a deep copy...
    
    SolveInformation info;
    synchronized(solver::solutionMutex) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        info.success = success ? 1 : timeout ? 2 : 0;
        
        if(success) {
            info.statesExploredBFS = totalsteps;
            info.timeBFS = chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - beginTime).count();
            
            deque<short> solutionPath;
            deque<int> indices;
            for(size_t index = totalsteps; index > 0; index = previousMove[index].first) {
                solutionPath.push_front(previousMove[index].second);
                indices.push_front(index);
            }
        
            info.solutionPath = solutionPath;
            info.shortestSolutionPath = solutionPath; //BFS solution = shortest solution
        
            addAdditionalToSolvePath(stateToBeSolved, solutionPath, game);
        }
        
        //22,27
    
        int layersWithoutSolution = -1;
        for(size_t index = totalsteps; index > 0; index = previousMove[index].first) {
            layersWithoutSolution++;
        }
        info.layersWithoutSolution = layersWithoutSolution;
        
        if(solver::solutionDP.count(solhash) != 0) {
            // cout << "READ4 " << solver::solutionDP.at(solhash).shortestSolutionPath.size() << endl;
            info = mergeSolveInformation(solver::solutionDP.at(solhash), info);
            
            // cout << "SET4: " << info.shortestSolutionPath.size() << " " << solhash << endl;
            /*
            for(const auto & a : game.currentState) {
                for(const auto & b : a) {
                    for(const auto & c : b)
                        cout << c << " ";
                    cout << endl;
                }
            }
             */
            solver::solutionDP.at(solhash) = info;
            // cout << "RES4: " << info.shortestSolutionPath.size() << endl;
        } else {
            solver::solutionDP.insert({solhash,info});
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    return info;
}



static float costEstimateFromGoal(const vvvs & currentState, const Game & game) {
    
    int estimate = 0; // 0 = win condition fulfilled.
    
    
    for(const auto & w : game.winConditionsNo)
        for(int y=0;y<game.currentLevelHeight;++y)
            for(int x=0;x<game.currentLevelWidth;++x)
                if(currentState[w.second][y][x] == w.first)
                    estimate += 1;
    
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
            if(!success) estimate += 1;
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
                                estimate += 1;
                            }
                        }
                    }
                }
            }
        }
    }
    
    
    // now compute for each LHS the closest RHS and take the distance as a function
    
    for(int i = 0; i < game.winConditionsSomeXOnYLHS.size(); ++i) {
        vector<pair<int,int> > lhsMatches, rhsMatches;
        for(int l=0; l<game.winConditionsSomeXOnYLHS[i].size(); ++l) {
            int lindex = game.winConditionsSomeXOnYLHS[i][l].first;
            int llayer = game.winConditionsSomeXOnYLHS[i][l].second;
            
            
            for(int y=0;y<game.currentLevelHeight;++y) {
                for(int x=0;x<game.currentLevelWidth;++x) {
                    if(currentState[llayer][y][x] == lindex) {
                        lhsMatches.push_back({x,y});
                    }
                }
            }
        }
        
        for(int r=0; r<game.winConditionsSomeXOnYRHS[i].size(); ++r) {
            int rindex = game.winConditionsSomeXOnYRHS[i][r].first;
            int rlayer = game.winConditionsSomeXOnYRHS[i][r].second;
            
            for(int y=0;y<game.currentLevelHeight;++y) {
                for(int x=0;x<game.currentLevelWidth;++x) {
                    if(currentState[rlayer][y][x] == rindex) {
                        rhsMatches.push_back({x,y});
                    }
                }
            }
        }
        
        
        //now add the min square distance for each:
        float minDist = FLT_MAX;
        for(auto X : lhsMatches) {
            for(auto Y : rhsMatches) {
                float dist = abs(X.first-Y.first) + abs(X.second-Y.second); //grid distance, can also take eulerian distance maybe?
                if(dist < minDist) minDist = dist;
            }
        }
        
        estimate += minDist; // since only some X on Y we only consider the smallest distance pair.
    }
    
    
    
    // now compute for all LHS the closest RHS and take the distance as a function
    
    for(int i = 0; i < game.winConditionsAllXOnYLHS.size(); ++i) {
        vector<pair<int,int> > lhsMatches, rhsMatches;
        for(int l=0; l<game.winConditionsAllXOnYLHS[i].size(); ++l) {
            int lindex = game.winConditionsAllXOnYLHS[i][l].first;
            int llayer = game.winConditionsAllXOnYLHS[i][l].second;
            
            
            for(int y=0;y<game.currentLevelHeight;++y) {
                for(int x=0;x<game.currentLevelWidth;++x) {
                    if(currentState[llayer][y][x] == lindex) {
                        lhsMatches.push_back({x,y});
                    }
                }
            }
        }
        
        for(int r=0; r<game.winConditionsAllXOnYRHS[i].size(); ++r) {
            int rindex = game.winConditionsAllXOnYRHS[i][r].first;
            int rlayer = game.winConditionsAllXOnYRHS[i][r].second;
            
            
            for(int y=0;y<game.currentLevelHeight;++y) {
                for(int x=0;x<game.currentLevelWidth;++x) {
                    if(currentState[rlayer][y][x] == rindex) {
                        rhsMatches.push_back({x,y});
                    }
                }
            }
        }
        
        /*
         for(const auto & X : lhsMatches) {
         float minDist = 1e7;
         int prevtaken = -1;
         for(const auto & Y : rhsMatches) {
         float dist = abs(X.first-Y.first) + abs(X.second-Y.second); //grid distance, can also take eulerian distance maybe?
         if(dist < minDist) minDist = dist;
         }
         estimate += minDist; // since all X on Y we need to add a penalty for every X.
         }*/
        
        //find a minimal matching (not a minimum, as this is a bit expensive to compute), but it's at most twice as bad as the optimal solution. Thus we can divide the result by 2 and will not overestimate then.
        vector<bool> taken (rhsMatches.size(), false);
        vector<int> lindices (lhsMatches.size());
        for(int l=0;l<lhsMatches.size();++l) lindices[l] = l;
        random_shuffle(lindices.begin(),lindices.end()); //make sure there's no skew
        for(int l : lindices) {
            const auto & X = lhsMatches[l];
            float minDist = 1e7;
            int prevtaken = -1;
            for(int r=0;r<rhsMatches.size();++r) {
                const auto & Y = rhsMatches[r];
                float dist = abs(X.first-Y.first) + abs(X.second-Y.second); //grid distance, can also take eulerian distance maybe?
                if(dist < minDist && !taken[r]) {
                    minDist = dist;
                    taken[r] = true;
                    if(prevtaken != -1) taken[prevtaken] = false;
                    prevtaken = r;
                }
            }
            if(minDist == 1e7) cout << "Seems unsolvable?" << endl;
            estimate += minDist; // since all X on Y we need to add a penalty for every X.
        }
    }
    
    return estimate;
}


/*
 static float bipartiteEstimateFromGoal(const vvvs & currentState, const Game & game) {
 
 int estimate = 0; // 0 = win condition fulfilled.
 
 
 for(const auto & w : game.winConditionsNo)
 for(int y=0;y<game.currentLevelHeight;++y)
 for(int x=0;x<game.currentLevelWidth;++x)
 if(currentState[w.second][y][x] == w.first)
 estimate += 1;
 
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
 if(!success) estimate += 1;
 }
 }
 
 //faster way of checking No X on Y:
 for(int i = 0; i<game.winConditionsNoXOnYLHS.size(); ++i) {
 for(int l=0; l<game.winConditionsSomeXOnYLHS[i].size();++l) {
 int lindex = game.winConditionsNoXOnYLHS[i][l].first;
 int llayer = game.winConditionsNoXOnYLHS[i][l].second;
 for(int y=0;y<game.currentLevelHeight;++y) {
 for(int x=0;x<game.currentLevelWidth;++x) {
 if(currentState[llayer][y][x] == lindex) {
 for(int r=0; r<game.winConditionsNoXOnYRHS[i].size(); ++r) { //for every LHS check that it is not on a RHS.
 int rindex = game.winConditionsNoXOnYRHS[i][r].first;
 int rlayer = game.winConditionsNoXOnYRHS[i][r].second;
 
 if(currentState[llayer][y][x] == lindex) {
 estimate += 1;
 }
 }
 }
 }
 }
 }
 }
 
 
 // now compute for each LHS the closest RHS and take the distance as a function
 
 for(int i = 0; i < game.winConditionsSomeXOnYLHS.size(); ++i) {
 vector<pair<int,int> > lhsMatches, rhsMatches;
 for(int l=0; l<game.winConditionsSomeXOnYLHS[i].size(); ++l) {
 int lindex = game.winConditionsSomeXOnYLHS[i][l].first;
 int llayer = game.winConditionsSomeXOnYLHS[i][l].second;
 
 
 for(int y=0;y<game.currentLevelHeight;++y) {
 for(int x=0;x<game.currentLevelWidth;++x) {
 if(currentState[llayer][y][x] == lindex) {
 lhsMatches.push_back({x,y});
 }
 }
 }
 }
 
 for(int r=0; r<game.winConditionsSomeXOnYRHS[i].size(); ++r) {
 int rindex = game.winConditionsSomeXOnYRHS[i][r].first;
 int rlayer = game.winConditionsSomeXOnYRHS[i][r].second;
 
 
 for(int y=0;y<game.currentLevelHeight;++y) {
 for(int x=0;x<game.currentLevelWidth;++x) {
 if(currentState[rlayer][y][x] == rindex) {
 rhsMatches.push_back({x,y});
 }
 }
 }
 }
 
 
 //now add the min square distance for each:
 float minDist = FLT_MAX;
 for(auto X : lhsMatches) {
 for(auto Y : rhsMatches) {
 float dist = abs(X.first-Y.first) + abs(X.second-Y.second); //grid distance, can also take eulerian distance maybe?
 if(dist < minDist) minDist = dist;
 }
 }
 
 estimate += minDist; // since only some X on Y we only consider the smallest distance pair.
 }
 
 
 
 // now compute for each LHS the closest RHS and take the distance as a function
 
 //since ALL X on Y is of the special form that no two X can be on the same Y then we can use a minimal bipartite matching for a more accurate heuristic
 //this idea stems from the sokoban solver
 //We can use the hungarian method in matrix fashion for this since it's a complete bipartite graph.
 
 for(int i = 0; i < game.winConditionsAllXOnYLHS.size(); ++i) {
 vector<pair<int,int> > lhsMatches, rhsMatches;
 for(int l=0; l<game.winConditionsAllXOnYLHS[i].size(); ++l) {
 int lindex = game.winConditionsAllXOnYLHS[i][l].first;
 int llayer = game.winConditionsAllXOnYLHS[i][l].second;
 
 
 for(int y=0;y<game.currentLevelHeight;++y) {
 for(int x=0;x<game.currentLevelWidth;++x) {
 if(currentState[llayer][y][x] == lindex) {
 lhsMatches.push_back({x,y});
 }
 }
 }
 }
 
 for(int r=0; r<game.winConditionsAllXOnYRHS[i].size(); ++r) {
 int rindex = game.winConditionsAllXOnYRHS[i][r].first;
 int rlayer = game.winConditionsAllXOnYRHS[i][r].second;
 
 
 for(int y=0;y<game.currentLevelHeight;++y) {
 for(int x=0;x<game.currentLevelWidth;++x) {
 if(currentState[rlayer][y][x] == rindex) {
 rhsMatches.push_back({x,y});
 }
 }
 }
 }
 
 
 //build the min cost matrix
 
 vector<vector<float> > cost(lhsMatches.size(), vector<float> (rhsMatches.size() ) );
 for(int l=0;l<lhsMatches.size();++l) {
 for(int r=0;r<rhsMatches.size();++r) {
 float dist = abs(lhsMatches[l].first-rhsMatches[r].first) + abs(lhsMatches[l].second-rhsMatches[r].second);
 cost[l][r] = dist;
 }
 }
 
 
 
 
 
 
 }
 
 return estimate;
 }*/


SolveInformation heuristicSolver(const vvvs & stateToBeSolved, const Game & game, int memoizationMaxSize, long long maxMillis,
                                 const deque<short> & additionalZeroCostMoves, bool dpLookup,
                                 volatile std::atomic_bool & keepSolving) {
    uint64_t solhash = game.getHash(); HashVVV(stateToBeSolved, solhash);
    
    if(dpLookup) {
        SolveInformation known;
        synchronized(solver::solutionMutex) {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            if(solver::solutionDP.count(solhash) != 0)
                known = solver::solutionDP.at(solhash);
            std::atomic_thread_fence(std::memory_order_seq_cst);
        }
        if(known.success == 0) return known; //unsolvable
        if(known.success == 1 && known.statesExploredHeuristic > -1) return known; //solution already found
    }
    
    set<vvvs> computed = {stateToBeSolved};
    
    priority_queue<pair<pair<float,int> ,vvvs>, vector<pair<pair<float,int> ,vvvs> >, greater<pair<pair<float,int> ,vvvs> > > qstates;
    qstates.push({{0,0},stateToBeSolved});
    
    int memoizationSize = memoizationMaxSize >- 1 ? MIN(10007, memoizationMaxSize) : 10007;
    vector<pair<size_t,short> > previousMove (memoizationSize+6); //used for reconstructing the solution.
    
    chrono::steady_clock::time_point beginTime = chrono::steady_clock::now();
    
    bool success = false, timeout = false;
    long long totalsteps = 0;
    vector<short> moves = {UP_MOVE,DOWN_MOVE,LEFT_MOVE,RIGHT_MOVE,ACTION_MOVE};
    for(size_t steps = 0; !qstates.empty(); ++steps) {
        if(totalsteps >= memoizationSize) {
            memoizationSize *= 2;
            previousMove.resize(memoizationSize+6);
        }
        
        if(!keepSolving || ((steps < 100 || totalsteps % 100 == 0) && (maxMillis >- 1 && chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - beginTime).count() > maxMillis))) {
            timeout = true;
            success = false;
            break;
        }
        
        if(totalsteps % 1000 == 0) {
            //cout << "computed " << to_string(totalsteps) << " steps in " << chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - beginTime).count() << "ms with current best cost: " << qstates.top().first.first << endl;
        }
        
        const vvvs cp = qstates.top().second;
        int stateIndex = qstates.top().first.second;
        qstates.pop();
        
        random_shuffle(moves.begin(), moves.end()); //shuffle a bit so there's no search direction skew
        
        //Loop unrolled for efficiency purposes
        for(short dir : moves) {
            vvvs c = cp;
            moveAndChangeField(dir, c, game);
            if(computed.count(c) == 0) {
                previousMove[++totalsteps] = {stateIndex,dir};
                computed.insert(c);
                float cost = costEstimateFromGoal(c, game);
                if(cost == 0) success = true;
                qstates.push({{cost,totalsteps}, c});
                if(success) break;
            }
        }
        if(success) break;
        
        if(totalsteps >= memoizationMaxSize && memoizationMaxSize >- 0) {
            timeout = true;
            success = false;
            break;
        }
    }
    
    //game.currentState = isfinal; // i thought this was a deep copy...
    while(qstates.size() > 2) qstates.pop();
	//cout << "fstate " << qstates.size() << endl;
	vvvs finalState = qstates.top().second;
	/*
	if (qstates.size() == 0) {
		SolveInformation info;
		return info;
	}
	finalState = qstates.top().second;
	*/
    
    SolveInformation info;
    synchronized(solver::solutionMutex) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        info.success = success ? 1 : timeout ? 2 : 0;
            
        if(success) {
            info.statesExploredHeuristic = MAX(totalsteps, info.statesExploredHeuristic);
            info.timeHeuristic = chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() -    beginTime).count();
            deque<short> solutionPath;
            for(size_t index = totalsteps; index > 0; index = previousMove[index].first) {
                solutionPath.push_front(previousMove[index].second);
            }
            if(info.solutionPath.size() == 0 || solutionPath.size() < info.solutionPath.size()) info.solutionPath = solutionPath;
        
            addAdditionalToSolvePath(stateToBeSolved, solutionPath, game);
        }
        
        if(solver::solutionDP.count(solhash) != 0) {
            // cout << "READ5 " << solver::solutionDP.at(solhash).shortestSolutionPath.size() << endl;
            info = mergeSolveInformation(solver::solutionDP.at(solhash), info);
            // cout << "SET5: " << info.shortestSolutionPath.size() << " " << solhash << endl;
            solver::solutionDP.at(solhash) = info;
            // cout << "RES5: " << info.shortestSolutionPath.size() << endl;
        } else {
            solver::solutionDP.insert({solhash,info});
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    return info;
}

struct AStarElem {
    float cost;
    long long index;
    float distanceFromOrig;
    vvvs state;
    float estimatedGoalCost;
};

bool operator<(const AStarElem & lhs, const AStarElem & rhs) {
    if(lhs.cost != rhs.cost) return lhs.cost > rhs.cost;
    return lhs.index > rhs.index;
}

SolveInformation astarSolver(const vvvs & stateToBeSolved, const Game & game, int memoizationMaxSize, long long maxMillis,
                             const deque<short> & additionalZeroCostMoves, bool dpLookup,
                             volatile std::atomic_bool & keepSolving) {
    uint64_t solhash = game.getHash(); HashVVV(stateToBeSolved, solhash);
    
    if(dpLookup) {
        SolveInformation known;
        synchronized(solver::solutionMutex) {
            std::atomic_thread_fence(std::memory_order_seq_cst);
            if(solver::solutionDP.count(solhash) != 0)
                known = solver::solutionDP.at(solhash);
            std::atomic_thread_fence(std::memory_order_seq_cst);
        }
        if(known.success == 0) return known; //unsolvable
        if(known.success == 1 && known.statesExploredAStar > -1) return known; //solution already found
    }
    
    //map<vvvs, float> openSet; openSet[stateToBeSolved] = 0;
    //set<vvvs> closedSet;
    
    //pair<pair<float,int>,pair<float,vvvs> > >=>  pair<pair<cost, distance>, pair<heuristic cost, vvvs> >
    priority_queue<AStarElem > qstates;
    float initialEstimatedGoalCost = costEstimateFromGoal(stateToBeSolved, game);
    AStarElem e {0.0, 0, 0, stateToBeSolved, initialEstimatedGoalCost};
    
    map<vvvs, float> visited; visited[stateToBeSolved] = initialEstimatedGoalCost;
    
    qstates.push(e);
    
    int memoizationSize = memoizationMaxSize >- 1 ? MIN(10007, memoizationMaxSize) : 10007;
    vector<pair<size_t,short> > previousMove (memoizationSize+6); //used for reconstructing the solution.
    
    chrono::steady_clock::time_point beginTime = chrono::steady_clock::now();
    
    bool success = false, timeout = false;
    long long totalsteps = 0;
    vector<short> moves = {UP_MOVE,DOWN_MOVE,LEFT_MOVE,RIGHT_MOVE,ACTION_MOVE};
    for(size_t steps = 0; !qstates.empty(); ++steps) {
        if(totalsteps >= memoizationSize) {
            memoizationSize *= 2;
            previousMove.resize(memoizationSize+6);
        }
        
        if(!keepSolving || ((steps < 100 || totalsteps % 100 == 0) && (maxMillis >- 1 && chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - beginTime).count() > maxMillis))) {
            success = false;
            timeout = true;
            break;
        }
        
        const AStarElem e = qstates.top();
        qstates.pop();
        //if(computed.count)
        
        //random_shuffle(moves.begin(), moves.end()); //shuffle a bit so there's no search direction skew
        
        if(visited.count(e.state) != -1) {
            visited[e.state] = -1; //added to closed set
            
            for(short dir : moves) {
                vvvs c = e.state;
                moveAndChangeField(dir, c, game);
                float lookedUpBestVal = std::numeric_limits<float>::infinity();
                if(visited.count(c) != 0) lookedUpBestVal = visited.at(c);
                if(lookedUpBestVal != -1) { // -1 = in closed set
                    float estimatedGoalCost = costEstimateFromGoal(c, game);
                    if(estimatedGoalCost == 0) success = true; //found goal
                    float finalcost = e.distanceFromOrig + estimatedGoalCost;
                    if(finalcost < lookedUpBestVal) {
                        visited[c] = finalcost;
                        //             cost,      index,      distFromOrig,         state, est. goal cost
                        previousMove[++totalsteps] = {e.index,dir};
                        AStarElem f = {finalcost, totalsteps, e.distanceFromOrig+1, c,     estimatedGoalCost};
                        qstates.emplace(f);
                        if(success) break;
                    }
                }
            }
            if(success) break;
        }
        
        if(totalsteps >= memoizationMaxSize && memoizationMaxSize >- 1) {
            timeout = true;
            success = false;
            break;
        }
    }
    
    
    SolveInformation info;
    synchronized(solver::solutionMutex) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        info.success = success ? 1 : timeout ? 2 : 0;

        if(success) {
            info.statesExploredAStar = totalsteps;
            info.timeAStar = chrono::duration_cast<std::chrono::milliseconds>(chrono::steady_clock::now() - beginTime).count();
            
            deque<short> solutionPath;
            for(size_t index = totalsteps; index > 0; index = previousMove[index].first) {
                solutionPath.push_front(previousMove[index].second);
            }
            info.solutionPath = solutionPath;
            addAdditionalToSolvePath(stateToBeSolved, solutionPath, game);
        }
        
        if(solver::solutionDP.count(solhash) != 0) {
            // cout << "READ6 " << solver::solutionDP.at(solhash).shortestSolutionPath.size() << endl;
            info = mergeSolveInformation(solver::solutionDP.at(solhash), info);
            // cout << "SET6: " << info.shortestSolutionPath.size() << " " << solhash << endl;
            solver::solutionDP.at(solhash) = info;
            // cout << "RES6: " << info.shortestSolutionPath.size() << endl;
        } else {
            solver::solutionDP.insert({solhash,info});
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    
    return info;
}




/*
 namespace mctssolver {
 static map <vvvs, float> mctsSolve;
 static Game game;
 
 struct Action {
 short dir;
 Action() {}
 Action(const short _dir) : dir(_dir) {}
 };
 
 class State {
 public:
 
 //--------------------------------------------------------------
 // MUST HAVE METHODS (INTERFACE)
 
 State(const vvvs & state) {
 data.state = state;
 data.depth = 0;
 cout << "mctsSolve" << endl;
 mctsSolve.clear();
 }
 
 // default constructors will do
 // copy and assignment operators should perform a DEEP clone of the given state
 //    State(const State& other);
 //    State& operator = (const State& other);
 
 
 // whether or not this state is terminal (reached end)
 bool is_terminal() const  {
 return costEstimateFromGoal(data.state, game) == 0;
 }
 
 //  agent id (zero-based) for agent who is about to make a decision
 int agent_id() const {
 return 0;
 }
 
 // apply action to state
 void apply_action(const Action& action)  {
 moveAndChangeField(action.dir, data.state, game);
 data.depth++;
 }
 
 
 // return possible actions from this state
 void get_actions(std::vector<Action>& actions) const {
 actions = vector<Action> { {UP_MOVE},{DOWN_MOVE},{LEFT_MOVE},{RIGHT_MOVE},{ACTION_MOVE} };
 }
 
 
 // get a random action, return false if no actions found
 bool get_random_action(Action& action) const {
 cout << "get_random_ation " << this << " " << data.depth  << endl;
 //if( mtcsExploredStates.count(data.state) != 0) return false;
 vector<Action> actions { {UP_MOVE},{DOWN_MOVE},{LEFT_MOVE},{RIGHT_MOVE},{ACTION_MOVE} };
 action = actions[ random() % actions.size() ];
 }
 
 
 // evaluate this state and return a vector of rewards (for each agent)
 // make sure to have an evaluate DP
 const vector<float> evaluate() const  {
 vector<float> rewards(1,0);
 rewards[0] = costEstimateFromGoal(data.state, game) == 0 ? 100 : 0;
 return rewards;
 }
 
 
 // return state as string (for debug purposes)
 std::string to_string() const {
 stringstream str;
 str << data.state.size() << endl;
 return str.str();
 }
 
 
 //--------------------------------------------------------------
 // IMPLEMENTATION SPECIFIC
 
 struct {
 vvvc state;
 int depth;
 //maybe add solve information from other solves to this.
 } data;
 };
 }
 */


//worst idea ever, mcts does not work for solving
/*
 SolveInformation mctsSolver(const vvvs & stateToBeSolved, const Game & game, int memoizationMaxSteps, long long maxMillis) {
 
 mctssolver::game = game;
 //init uct params
 mctssolver::State state (stateToBeSolved);
 mctssolver::Action action {STATIONARY_MOVE};
 msa::mcts::UCT<mctssolver::State, mctssolver::Action> uct;
 
 uct.uct_k = sqrt(2);
 uct.max_millis = 0;
 uct.max_iterations = 50000;
 uct.simulation_depth = 5;
 
 
 bool solutionFound = false;
 //while(!solutionFound) {
 // run uct mcts on current state and get best action
 action = uct.run(state);
 
 // apply the action to the current state
 state.apply_action(action);
 
 move(action.dir, currentGame);
 
 //    solutionFound = state.is_terminal();
 //}
 
 SolveInformation info;
 bool success = state.is_terminal();
 bool timeout = false;
 info.success = success ? 1 : timeout ? 2 : 0;
 info.statesExplored = -1; // only depth not
 info.numberOfLayersWithoutSolution = -1;
 info.solutionPath.clear();
 
 return info;
 }
 */


//Returns which solver is most suitable for a given level 0 = BFS, 1 = Greedy, 2 = AStar
int bestSolver(const vvvs & state, const Game & game) {
    uint64_t solhash = game.getHash(); HashVVV(state, solhash);
    SolveInformation info;
    synchronized(solver::solutionMutex) {
        std::atomic_thread_fence(std::memory_order_seq_cst);
        if(solver::solutionDP.count(solhash) != 0) {
            info = solver::solutionDP.at(solhash);
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
    if(info.success == 0) return 0; //if it's impossible, that means every state can be checked, which means BFS is usually the best.
    //assumes the best is astar with some penalty of 1000
    int bestS = 2, bestCompl = info.statesExploredAStar;
    
    if( info.statesExploredHeuristic != -1 && (bestCompl == -1 || info.statesExploredHeuristic + 500 < bestCompl) ) {
        bestS = 1;
        bestCompl = info.statesExploredHeuristic;
    }
    
    if(  info.statesExploredBFS != -1 && (bestCompl == -1 || info.statesExploredBFS + 500 < bestCompl) ) {
        bestS = 0;
        bestCompl = info.statesExploredBFS;
    }
       
    return bestS;
}

struct SolveThread {
    vvvs state;
    Game game;
    deque<short> initialMoves;
    volatile std::atomic_bool keepTryingToSolve;
    SolveThread(const vvvs & _state, const Game & _game, const deque<short> & _beginResources) : state(_state), game(_game), initialMoves(_beginResources) {
        keepTryingToSolve = true;
    }
    
    SolveThread(SolveThread&& o) noexcept : state(o.state), game(o.game), initialMoves(o.initialMoves), keepTryingToSolve(o.keepTryingToSolve.load()) { }
    
    SolveThread& operator=(SolveThread&&) { return *this; }
    
    //SolveThread(const SolveThread & st) : state(st.state), game(st.game), initialMoves(st.initialMoves), keepTryingToSolve(st.keepTryingToSolve) {}
    
    //SolveThread(SolveThread&& st) : state(st.state), game(st.game), initialMoves(st.initialMoves), keepTryingToSolve(st.keepTryingToSolve)  {}
};

static vector<SolveThread> sthreads;
static vector<thread> threads;

static void solveThread0(int ind, volatile std::atomic_bool & keepTryingToSolve, vvvs state, Game cgame, deque<short> initialMoves) {
#ifdef _WIN32
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
	cerr << "START SOLVING THREAD 0,"<< ind << " " << keepTryingToSolve << endl;
    vvvs movedState = state;
    for(short move : initialMoves) moveAndChangeField(move, movedState, cgame);
    SolveInformation info = bfsSolver(movedState, cgame, -1, 1000*1000*1000, gbl::emptyMoves, true, keepTryingToSolve);
    if(info.success == 1) {
        deque<short> combinedMoves = initialMoves;
        combinedMoves.insert(std::end(combinedMoves), std::begin(info.solutionPath), std::end(info.solutionPath));
        addAdditionalToSolvePath(state, combinedMoves, cgame);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);

    cerr << "STOP SOLVING THREAD 0," << ind << endl;
}
static void solveThread1(int ind, volatile std::atomic_bool & keepTryingToSolve, vvvs state, Game cgame, deque<short> initialMoves) {
#ifdef _WIN32
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
	cerr << "START SOLVING THREAD 1,"<< ind << " " << keepTryingToSolve << endl;
    vvvs movedState = state;
    for(short move : initialMoves) moveAndChangeField(move, movedState, cgame);
    SolveInformation info = heuristicSolver(movedState, cgame, -1, 1000*1000*1000, gbl::emptyMoves, true, keepTryingToSolve);
    if(info.success == 1) {
        deque<short> combinedMoves = initialMoves;
        combinedMoves.insert(std::end(combinedMoves), std::begin(info.solutionPath), std::end(info.solutionPath));
        addAdditionalToSolvePath(state, combinedMoves, cgame);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);

    cerr << "STOP SOLVING THREAD 1," << ind << endl;
}
static void solveThread2(int ind, volatile std::atomic_bool & keepTryingToSolve, vvvs state, Game cgame, deque<short> initialMoves) {
#ifdef _WIN32
//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
	cerr << "START SOLVING THREAD 2,"<< ind << " " << keepTryingToSolve << endl;
    vvvs movedState = state;
    for(short move : initialMoves) moveAndChangeField(move, movedState, cgame);
    SolveInformation info = astarSolver(movedState, cgame, -1, 1000*1000*1000, gbl::emptyMoves, true, keepTryingToSolve);
    if(info.success == 1) {
        deque<short> combinedMoves = initialMoves;
        combinedMoves.insert(std::end(combinedMoves), std::begin(info.solutionPath), std::end(info.solutionPath));
        addAdditionalToSolvePath(state, combinedMoves, cgame);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);

    cerr << "STOP SOLVING THREAD 2," << ind << endl;
}
//only thread safe under the assumption that startSolving/stopSolving get called by the main thread
//static if from D could be used nicely here
void startSolving(int ind, const vvvs & state, const Game & game, const deque<short> & initialMoves ) {
    //return;
    if(ind < sthreads.size() && sthreads[ind].keepTryingToSolve && sthreads[ind].game.getHash() == game.getHash() && sthreads[ind].state == state && sthreads[ind].initialMoves == initialMoves) return; //can continue as it's still running normally
    
    //FUTURE TODO: Currently only works if startSolving gets called with ind in order (0,1,...)
    stopSolving(ind);
    cout << "LOADING IND: " << ind << " " << game.rules.size() << endl;
    if(ind == sthreads.size())
        sthreads.push_back(SolveThread(state, game, initialMoves));
    else if(ind < sthreads.size()) {
        //SolveThread st(state, game, initialMoves);
        //cout << "TRUE?? " << st.keepTryingToSolve << endl;
        cout << "TRUE?? " << sthreads.at(ind).keepTryingToSolve << " " << endl;
        sthreads.at(ind).state = state;
        sthreads.at(ind).game = game;
        sthreads.at(ind).initialMoves = initialMoves;
        sthreads.at(ind).keepTryingToSolve = true;
        cout << "TRUE2?? " << sthreads.at(ind).keepTryingToSolve << " " << endl;
    }
    else
        assert(false);
    
    cout << "AFTER LOADING IND: " << ind << " " << sthreads[ind].game.rules.size() << endl;

    thread t0(solveThread0, ind, ref(sthreads[ind].keepTryingToSolve), state, game, initialMoves);
    thread t1(solveThread1, ind, ref(sthreads[ind].keepTryingToSolve), state, game, initialMoves);
    thread t2(solveThread2, ind, ref(sthreads[ind].keepTryingToSolve), state, game, initialMoves);

#ifdef BLABLABLANOTDEFINED
	SetThreadPriority(t0, THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(t1, THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(t2, THREAD_PRIORITY_ABOVE_NORMAL);//(THREAD_PRIORITY_ABOVE_NORMAL);
#endif

    if(3*ind+2 >= threads.size()) {
        threads.push_back(move(t0));
        threads.push_back(move(t1));
        threads.push_back(move(t2));
    } else {
        threads[3*ind+0] = move(t0);
        threads[3*ind+1] = move(t1);
        threads[3*ind+2] = move(t2);
    }
}

void stopSolving(int ind) {
    cout << "FORCE STOPPING" << endl;
    if(ind < sthreads.size()) sthreads[ind].keepTryingToSolve = false;
    std::atomic_thread_fence(std::memory_order_seq_cst);
    for(int i=3*ind; i<3*ind+3;++i)
        if(i < threads.size() && threads[i].joinable())
            threads[i].join();
    cout << "DONE FORCE STOPPING" << endl;
}
