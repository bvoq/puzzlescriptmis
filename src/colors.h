#ifndef colors_h
#define colors_h

#include "macros.h"

namespace colors {
    extern map<string, ofColor> palette;
    
    extern ofColor colorIDE_TEXT;
    extern ofColor colorIDE_BACKGROUND;
    extern ofColor colorIDE_CURSOR;
    extern ofColor colorBACKGROUND;
    
    extern ofColor colorLEVEL_EDITOR_BG;
    extern ofColor colorEXPLOITATION_BG;
    extern ofColor colorINSPIRATION_BG;
    
    extern set<string> paletteStrs;
    
    extern map<uint64_t, short> textureMap;
    extern vector<ofTexture> textures;
}

extern bool switchToPalette(string paletteStr);
extern void switchToDefaultPalette();

#endif /* colors_h */
