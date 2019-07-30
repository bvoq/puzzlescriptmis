/*
 MUST READ:
 ==========
 The structure of the code follows a bit of my own C++ style. Every header file only includes this file 'macros.h'.
 This way, if you include a header you don't automatically include other headers recursively.
 As a downside all enums which are passed along to functions have to be defined in this file and all structs/classes which are passed along functions need to be declared in a forward fashion in this file.
 */


/*
 TODO:
 - UNDO BUTTON IN LEVEL EDITOR
 - DP IN ASTARSOLVER (if level switched, clear)
 - Make level switchable
 - Start solving while presenting the level
 
*/

#pragma once

#include "bitsstdc.h" //Include all C++ libraries, see bitsstdc.h for the list of included libraries.

#ifndef compiledWithoutOpenframeworks
#include "ofMain.h"
inline long long getAdjustedTime() {
    return ofGetElapsedTimeMicros();
}
#endif

using namespace std; //For every source file, namespace std is required!

#define vvvs std::vector<std::vector<std::vector<short> > > //vvvs is a short-hand indicating a level instance.
#define vvvc std::vector<std::vector<std::vector<short> > > //vvvc is a short-hand indicating a level instance.

#define synchronized(m) \
for(unique_lock<recursive_mutex> lk(m); lk; lk.unlock())

const short STATIONARY_MOVE       = (short) 0b0000000;
const short UP_MOVE               = (short) 0b0000001;
const short DOWN_MOVE             = (short) 0b0000010;
const short LEFT_MOVE             = (short) 0b0000100;
const short RIGHT_MOVE            = (short) 0b0001000;
const short VERTICAL_MOVE         = (short) 0b0000011;
const short HORIZONTAL_MOVE       = (short) 0b0001100;
const short ORTHOGONAL_MOVE       = (short) 0b0001111;
const short ACTION_MOVE           = (short) 0b0010000;
const short RE_MOVE               = (short) 0b0100000;
const short NO_OR_ORTHOGONAL_MOVE = (short) 0b1000000;


// HASHING MACROS
#define FNV64(data,start) ((start^data)*((uint64_t)1099511628211ULL))
const uint64_t INITIAL_HASH = 14695981039346656037ULL;

// DEBUG MACRO, can remove all debug statements by commenting out the other line:
#define DEB(x) cerr << x << endl;
//#define DEB(x) while(0) cerr << x << endl;
//#define MIN(a,b) (((a)<(b))?(a):(b))
//#define MAX(a,b) (((a)>(b))?(a):(b))

#define HashVVV(vvvarr,chash)\
for(const auto & x : vvvarr) { \
    for(const auto & y : x) { \
        for(const auto & z : y) { \
            chash = FNV64(z, chash);\
        }\
    }\
}


#define cout if(false) cout


// Forward declare all structs (which are used in multiple header files) in C++ fashion
struct Rule;
struct Game;
struct Record;
struct Logger;

// ALL ENUMS
enum WIN_CONDITION_TYPE {
    WIN_CONDITION_NO_PROPERTY, WIN_CONDITION_NO_AGGREGATE,
    WIN_CONDITION_SOME_PROPERTY, WIN_CONDITION_SOME_AGGREGATE,
    WIN_CONDITION_SOME_ON_PROPERTY, WIN_CONDITION_ON_AGGREGATE,
    WIN_CONDITION_ALL_ON_PROPERTY, WIN_CONDITION_ALL_ON_AGGREGATE
};


enum MODE_TYPE {
    MODE_LEVEL_EDITOR, MODE_PLAYING, MODE_MESSAGE, MODE_EXPLOITATION, MODE_INSPIRATION, MODE_UNKNOWN
};

enum KEY_TYPE {
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ACTION, KEY_WIN, KEY_UNDO, KEY_RESTART, KEY_SOLVE, KEY_PRINT, KEY_IMPORT, KEY_GENERATE
};
