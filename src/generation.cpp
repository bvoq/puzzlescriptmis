#include "generation.h"

//#include "ofxMSAmcts.h"

#include "engine.h"
#include "game.h"
#include "global.h"
#include "solver.h"
#include "visualsandide.h"

namespace generator {
    recursive_mutex generatorMutex;
    vector<set<pair<float,vvvs> > > generatorNeighborhood;
    int counter = 0, solvedCounter = 0, unsolvableCounter = 0, timedoutCounter = 0, maxSolveTime = 0;
}

/*
using namespace msa::mcts;
namespace generator {
    
    recursive_mutex generatorMutex;
    vector<set<pair<float,vvvs> > > generatorNeighborhood;

    struct Action {
        Action() {}
        Action(const vvvs & _newState) : newState(_newState) {}
        vvvs newState;
    };
    
    class State {
    public:
        
        //--------------------------------------------------------------
        // MUST HAVE METHODS (INTERFACE)
        
        int maxStates = 10;
        
        
        State() {
            //generator::generatorNeighborhood[gbl::currentGame.currentLevelIndex].clear();
        }
        
        // default constructors will do
        // copy and assignment operators should perform a DEEP clone of the given state
        //    State(const State& other);
        //    State& operator = (const State& other);
        
        
        // whether or not this state is terminal (reached end)
        bool is_terminal() const  {
            return false; //there is no such thing as the perfect level (at least not obviously)
        }
        
        //  agent id (zero-based) for agent who is about to make a decision
        int agent_id() const {
            return 0;
        }
        
        // apply action to state
        void apply_action(const Action& action)  {
            data.state = action.newState;
        }
        
        
        // return possible actions from this state
        void get_actions(std::vector<Action>& actions) const {
 
            //vector<vvvs> newStates = generateStep(data.state, maxStates, gbl::currentGame) ;
            ////TODO: make sure none of them previously explored.
            //for(int i=0;i<newStates.size();++i) {
            //    actions.emplace_back(newStates[i]);
            //}
        }
        
        
        // get a random action, return false if no actions found
        bool get_random_action(Action& action) const {
 
            //vector<vvvs> randomState = generateStep(data.state, 1, gbl::currentGame);
            //action.newState = randomState[0];
 
        }
        
        
        // evaluate this state and return a vector of rewards (for each agent)
        // make sure to have an evaluate DP
        const vector<float> evaluate() const  {
            vector<float> rewards(1,0);
            atomic_bool keepSolving(true);
            deque<short> emptyMoves = {};
            SolveInformation info = astarSolver(data.state, gbl::currentGame, 10000, 5000, emptyMoves, keepSolving);
            //TODO: Penalty according to time.
            if(info.success == 1) {
                rewards[0] = info.statesExploredAStar;
                //gbl::currentGame.generatorNeighborhood.insert( make_pair(rewards[0], data.state) );
            }
            return rewards;
        }
        
        
        // return state as string (for debug purposes)
        std::string to_string() const  {
            stringstream str;
            str << data.state.size() << endl;
            return str.str();
        }
        
        
        //--------------------------------------------------------------
        // IMPLEMENTATION SPECIFIC
        
        struct {
            vvvc state;
            //maybe add solve information from other solves to this.
        } data;
        
        void reset() {
            
        }
    };
}*/



static volatile std::atomic_bool requestGenerating(false);
static Game cgame;
static vector<vector<bool> > cmodifyTable;
static void generating() {
    cerr << "START GENERATING THREAD" << endl;

    int solverID = bestSolver(cgame.currentState, cgame);
    
    chrono::steady_clock::time_point timeSinceLastImprovement = chrono::steady_clock::now();
    
    //default time
    long long timeToSolve = 100; //start with a tenth of a second
    long long statesForExploitation;
    
    
    uint64_t solhash = gbl::currentGame.getHash(); HashVVV(gbl::currentGame.beginStateAfterStationaryMove, solhash);
    synchronized(solver::solutionMutex) {
        if(solver::solutionDP.count(solhash) != 0) {
            SolveInformation inf = solver::solutionDP.at(solhash);
            long long initialTime = -1;
            if(inf.timeBFS>-1) initialTime = MAX(initialTime, inf.timeBFS);
            if(inf.timeAStar>-1) initialTime = MAX(initialTime, inf.timeAStar);
            if(inf.timeHeuristic>-1) initialTime = MAX(initialTime, inf.timeHeuristic);
            if(initialTime != -1) timeToSolve = initialTime * 4;
        }
    }
    bool hasFoundAnyTransform = false;
    generator::counter = 0; generator::solvedCounter = 0, generator::unsolvableCounter = 0, generator::timedoutCounter = 0, generator::maxSolveTime = timeToSolve;
    vector<vvvs> unknownStack;
    while(requestGenerating) {
        generator::counter++;
        //if(counter % 10 == 0) cerr << "generated " << counter << " levels." << endl;
        if(!hasFoundAnyTransform) {
            chrono::steady_clock::time_point ctime = chrono::steady_clock::now();
            auto timePassedSinceLastImprovement = chrono::duration_cast<std::chrono::milliseconds>(ctime - timeSinceLastImprovement).count();
            timeToSolve = MAX(timeToSolve, timePassedSinceLastImprovement/10);
        }
        solverID = bestSolver(cgame.currentState, cgame); //update best known solver
        
        
        /* maybe remove possibility of double match (think about this some more...) */
        vector<vvvs> newStates = generateStep(cgame.currentState, 1, cgame, cmodifyTable);
        //Do the obligatory stationary move
        moveAndChangeField(STATIONARY_MOVE, newStates[0], cgame);

        //TODO: only matched part
        //TODO: make sure newStates doesn't appear in current state
        
        if(newStates.size() > 0) {
            SolveInformation info;
            
            if(solverID == 0)
                info = bfsSolver(newStates[0], cgame, -1, timeToSolve, gbl::emptyMoves, true, requestGenerating);
            else if(solverID == 1)
                info = heuristicSolver(newStates[0], cgame, -1, timeToSolve, gbl::emptyMoves, true, requestGenerating);
            else if(solverID == 2)
                info = astarSolver(newStates[0], cgame, -1, timeToSolve, gbl::emptyMoves, true, requestGenerating);
            
            if(info.success == 1) {
                long long timeItTook = LLONG_MAX;
                timeItTook = MIN(timeItTook, info.timeBFS == -1 ? LLONG_MAX : info.timeBFS);
                timeItTook = MIN(timeItTook, info.timeHeuristic == -1 ? LLONG_MAX : info.timeHeuristic);
                timeItTook = MIN(timeItTook, info.timeAStar == -1 ? LLONG_MAX : info.timeAStar);
                long long statesExplored = LLONG_MAX;
                statesExplored = MIN(statesExplored, info.statesExploredBFS == -1 ? LLONG_MAX : info.statesExploredBFS);
                statesExplored = MIN(statesExplored, info.statesExploredHeuristic == -1 ? LLONG_MAX : info.statesExploredHeuristic);
                statesExplored = MIN(statesExplored, info.statesExploredAStar == -1 ? LLONG_MAX : info.statesExploredAStar);
                if(statesExplored == LLONG_MAX || timeItTook == LLONG_MAX) continue;
                if(info.statesExploredBFS == -1) {
                    SolveInformation infoBFS = bfsSolver(newStates[0], cgame, statesExplored+6, -1, gbl::emptyMoves, false, requestGenerating);
                    if(infoBFS.statesExploredBFS != -1)
                        statesExplored = MIN(statesExplored, infoBFS.statesExploredBFS);
                }
                if(info.statesExploredHeuristic == -1) {
                    SolveInformation infoGreedy = heuristicSolver(newStates[0], cgame, statesExplored+6, -1, gbl::emptyMoves, false, requestGenerating);
                    if(infoGreedy.statesExploredHeuristic != -1)
                        statesExplored = MIN(statesExplored, infoGreedy.statesExploredHeuristic);
                }
                if(info.statesExploredAStar == -1) {
                    SolveInformation infoAStar = astarSolver(newStates[0], cgame, statesExplored+6, -1, gbl::emptyMoves, false, requestGenerating);
                    if(infoAStar.statesExploredAStar != -1)
                        statesExplored = MIN(statesExplored, infoAStar.statesExploredAStar);
                }
                if(statesExplored < 0) continue;
                
                auto p = make_pair(-statesExplored, newStates[0] );
                bool addTime = false;
                synchronized(generator::generatorMutex) {
                    std::atomic_thread_fence(std::memory_order_seq_cst);
                    if(generator::generatorNeighborhood[cgame.currentLevelIndex].empty() || -statesExplored < generator::generatorNeighborhood[cgame.currentLevelIndex].rbegin()->first)
                        addTime = true;
                    generator::solvedCounter++;
                    generator::generatorNeighborhood[cgame.currentLevelIndex].insert( p );
                    if(generator::generatorNeighborhood[cgame.currentLevelIndex].size() > 4) {
                        generator::generatorNeighborhood[cgame.currentLevelIndex].erase(    prev(generator::generatorNeighborhood[cgame.currentLevelIndex].end()) );
                    }
                }
                if(addTime) {
                    //cout << "alrighty " << timeToSolve << " " << timeItTook << endl;
                    if(!hasFoundAnyTransform)
                        timeToSolve = MAX(1000,timeItTook*7);
                    else {
                        timeToSolve = MAX(timeToSolve, timeItTook*7);
                        //cerr << "time to solve: " <<  timeToSolve << endl;
                    }
                    
                    generator::maxSolveTime = timeToSolve;
                    hasFoundAnyTransform = true;
                }
            } else if(info.success == 0) {
                generator::unsolvableCounter++;
            }else if(info.success == 2) {
                generator::timedoutCounter++;
                /* In case you also want to add the unsolved levels:
                pair<long long, vvvs> p;
                synchronized(generator::generatorMutex) {
                    std::atomic_thread_fence(std::memory_order_seq_cst);
                    if(generator::generatorNeighborhood[cgame.currentLevelIndex].size()==4 ) {
                        auto it = generator::generatorNeighborhood[cgame.currentLevelIndex].begin();
                        it++;
                        long long last = it->first;
                        p = make_pair(last, newStates[0] );
                        generator::generatorNeighborhood[cgame.currentLevelIndex].insert( p );
                        if(generator::generatorNeighborhood[cgame.currentLevelIndex].size() > 4) {
                            generator::generatorNeighborhood[cgame.currentLevelIndex].erase(    prev(generator::generatorNeighborhood[cgame.currentLevelIndex].end()) );
                        }
                    }
                }*/
                //unknownStack.push_back(newStates[0]);
                //cerr << "ended with timeout " << info.success << " " << hasFoundAnyTransform << " " << timeToSolve << endl;
            }
        }
        
        //cout << "try generating next" << endl;
    }
    cerr << "STOP GENERATING THREAD" << endl;
}

static volatile bool stillGenerating = false;
static int generatorCount = 1;
static vector<thread> generatorThread;
void startGenerating() {
    cout << "start generating" << endl;
    if(stillGenerating && cgame.getHash() == gbl::currentGame.getHash() && cmodifyTable == editor::modifyTable[gbl::currentGame.currentLevelIndex] && cgame.currentState == gbl::currentGame.currentState) return;
    if(stillGenerating) stopGenerating();
    stillGenerating = true;
    
    requestGenerating = true;
    cgame = gbl::currentGame;
    cmodifyTable = editor::modifyTable[gbl::currentGame.currentLevelIndex];
    generatorCount = MAX(1,(int)thread::hardware_concurrency() - 1);
    //Do the obligatory empty stationary move
    generatorThread.resize(generatorCount);
    for(int i=0; i<generatorThread.size();++i) {
        generatorThread[i] = thread(generating);
    }
    
    //launch C++ thread
}

void stopGenerating() {
    cout << "stop generating" << endl;
    requestGenerating = false;
    std::atomic_thread_fence(std::memory_order_seq_cst);
    if(stillGenerating) {
        for(size_t i=0;i<generatorThread.size();++i) {
            generatorThread[i].join();
        }
    }
    
    if(!generator::generatorNeighborhood[gbl::currentGame.currentLevelIndex].empty()) {
        generator::generatorNeighborhood[gbl::currentGame.currentLevelIndex].clear();
        //the mouse needs to be set to false since clearing the generated levels skews the view and can lead to unintended mouse presses if not disabled
        //gbl::isFirstMousePressed = false;
        //gbl::isMousePressed = false;
    }
    stillGenerating = false;
    //optionally join?
}

bool stillTransforming() {
    return stillGenerating;
}
