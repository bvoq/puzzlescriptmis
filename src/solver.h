#ifndef solver_h
#define solver_h

#include "macros.h"

struct SolveInformation {
    deque<short> solutionPath; //any solution path
    deque<short> shortestSolutionPath; //shortest possible solution path (BFS solver solution)
    int success = 2; // success = 0 => no solution possible, success = 1 => found solution, success = 2 => timeout
    int layersWithoutSolution = -1; //The BFS solver can return that there is no solution within x number of moves
    long long statesExploredBFS = -1, statesExploredHeuristic = -1, statesExploredAStar = -1; //The maximum number of states any solver has explored for a given level.
    long long timeBFS=-1, timeHeuristic=-1, timeAStar=-1; //time for solution
};

namespace solver {
    extern recursive_mutex solutionMutex;
    //extern map<pair<uint64_t,vvvs>, SolveInformation> solutionDP;
    extern unordered_map<uint64_t, SolveInformation> solutionDP;
}

extern SolveInformation mergeSolveInformation(const SolveInformation & a, const SolveInformation & b);

extern SolveInformation bfsSolver(const vvvs & stateToBeSolved, const Game & game, int memoizationMaxSteps, long long maxMillis,
                                  const deque<short> & additionalZeroCostMoves, bool dpLookup,
                                  volatile std::atomic_bool & keepSolving);
extern SolveInformation heuristicSolver(const vvvs & stateToBeSolved, const Game & game, int memoizationMaxSteps, long long maxMillis,
                                        const deque<short> & additionalZeroCostMoves, bool dpLookup,
                                        volatile std::atomic_bool & keepSolving);
extern SolveInformation astarSolver(const vvvs & stateToBeSolved, const Game & game, int memoizationMaxSteps, long long maxMillis,
                                    const deque<short> & additionalZeroCostMoves, bool dpLookup,
                                    volatile std::atomic_bool & keepSolving);
//SolveInformation mctsSolver(const vvvs & stateToBeSolved, const Game & game, int memoizationMaxSteps, long long maxMillis);

extern int bestSolver(const vvvs & state, const Game & game);

//allocates threads that try to solve a problem. only thread safe if called by the main thread (one thread).
extern void startSolving(int startIndex, const vvvs & state, const Game & game, const deque<short> & initialMoves);
extern void stopSolving(int startIndex);

#endif /* solver_h */
