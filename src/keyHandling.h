#ifndef keyHandling_h
#define keyHandling_h

#include "macros.h"

namespace keyHandling {
    extern queue<pair<KEY_TYPE,long long> > keyQueue;
}

extern void initDefaultKeyMapping();
extern void keyHandle(int key);
extern void executeKeys();


#endif /* keyHandling_h */
