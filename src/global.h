#ifndef globals_h
#define globals_h

#include "macros.h"

namespace gbl {
    extern MODE_TYPE mode;
    extern Game currentGame;
    extern Record record;
    extern int version;
    extern const string locationOfResources;
    extern bool isMousePressed, isMouseReleased, isFirstMousePressed;
    extern const deque<short> emptyMoves;
}

extern void switchToLevel(int level, Game & game);
extern void display();

#endif /* globals_h */
