#ifndef engine_h
#define engine_h

#include "macros.h"

extern void moveAndChangeField(const short & moveDir, vvvs & currentState, const Game & game);
extern bool moveAndChangeFieldAndReturnWinningCondition(const short & moveDir, vvvs & currentState, const Game & game);
extern bool move(const short & moveDir, Game & game);
extern void undo(Game & game);
extern void restart(Game & game);

extern vector<vvvs> generateStep(const vvvs & prevState, int maxStates, const Game & game, const vector<vector<bool> > & modifyTable);

#endif /* engine_h */
