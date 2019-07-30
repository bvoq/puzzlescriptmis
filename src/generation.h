#ifndef generation_h
#define generation_h

#include "macros.h"

namespace generator {
    extern recursive_mutex generatorMutex;
    extern vector<set<pair<float,vvvs> > > generatorNeighborhood;
}
extern void startGenerating();
extern void stopGenerating();
extern bool stillTransforming();

#endif /* generation_h */
