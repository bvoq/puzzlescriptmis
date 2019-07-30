#ifndef levels_h
#define levels_h

#include "macros.h"

extern bool parseLevels(vector<string> lines, int lineFrom, int lineTo, Game & game, Logger & logger);
extern void changeLevelStr(vector<string> & levelstr, int level, vvvs newLevel, vvvs oldLevel, const Game & oldgame);

#endif /* levels_h */
