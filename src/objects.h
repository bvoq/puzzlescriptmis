#ifndef objects_h
#define objects_h

#include "macros.h"


/* Removed in favor of faster SoA layout.
struct ObjectInfo {
    //0000 0000 0000 0000 0000
    short index = -1; //the block index, starting from 0
    short layer = -1; //indicates which layer the block is.

    string primaryname; //used by the debugger to refer to it.
#ifndef compiledWithoutOpenframeworks
    ofPixels pixels;
    vector<ofColor> colors;
    ofTexture texture;
#endif
};*/

extern int PIXEL_RESOLUTION;

extern bool parseObjects(vector<string> lines, int lineFrom, int lineTo, Game & game, Logger & logger);

#endif /* objects_h */
