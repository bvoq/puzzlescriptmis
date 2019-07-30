#include "colors.h"

#ifndef compiledWithoutOpenframeworks


namespace colors {
    map<string, ofColor> palette;
    
    ofColor colorIDE_TEXT = ofColor(0xff,0xff,0xff);
    ofColor colorIDE_BACKGROUND = ofColor(0x11,0x18,0x28);
    ofColor colorIDE_CURSOR = ofColor(0xff,0x00,0x66);
    ofColor colorBACKGROUND = ofColor(0xDB,0xDB,0xDB);

    ofColor colorLEVEL_EDITOR_BG = ofColor(0x11,0x18,0x28);
    ofColor colorEXPLOITATION_BG = ofColor(0x28,0x18,0x11);
    ofColor colorINSPIRATION_BG  = ofColor(0x11,0x28,0x18);

    map<uint64_t, short> textureMap;
    vector<ofTexture> textures;
}

using namespace colors;

bool switchToPalette(string paletteStr) {
    
    if(paletteStr == "mastersystem") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x000000)},
            {"white",ofColor::fromHex(0xFFFFFF)},
            {"gray",ofColor::fromHex(0x555555)},
            {"darkgray",ofColor::fromHex(0x555500)},
            {"lightgray",ofColor::fromHex(0xAAAAAA)},
            {"grey",ofColor::fromHex(0x555555)},
            {"darkgrey",ofColor::fromHex(0x555500)},
            {"lightgrey",ofColor::fromHex(0xAAAAAA)},
            {"red",ofColor::fromHex(0xFF0000)},
            {"darkred",ofColor::fromHex(0xAA0000)},
            {"lightred",ofColor::fromHex(0xFF5555)},
            {"brown",ofColor::fromHex(0xAA5500)},
            {"darkbrown",ofColor::fromHex(0x550000)},
            {"lightbrown",ofColor::fromHex(0xFFAA00)},
            {"orange",ofColor::fromHex(0xFF5500)},
            {"yellow",ofColor::fromHex(0xFFFF55)},
            {"green",ofColor::fromHex(0x55AA00)},
            {"darkgreen",ofColor::fromHex(0x005500)},
            {"lightgreen",ofColor::fromHex(0xAAFF00)},
            {"blue",ofColor::fromHex(0x5555AA)},
            {"lightblue",ofColor::fromHex(0xAAFFFF)},
            {"darkblue",ofColor::fromHex(0x000055)},
            {"purple",ofColor::fromHex(0x550055)},
            {"pink",ofColor::fromHex(0xFFAAFF)}
        };
    } else if(paletteStr == "gameboycolour") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x000000)},
            {"white",ofColor::fromHex(0xFFFFFF)},
            {"grey",ofColor::fromHex(0x7F7F7C)},
            {"darkgrey",ofColor::fromHex(0x3E3E44)},
            {"lightgrey",ofColor::fromHex(0xBAA7A7)},
            {"gray",ofColor::fromHex(0x7F7F7C)},
            {"darkgray",ofColor::fromHex(0x3E3E44)},
            {"lightgray",ofColor::fromHex(0xBAA7A7)},
            {"red",ofColor::fromHex(0xA7120C)},
            {"darkred",ofColor::fromHex(0x880606)},
            {"lightred",ofColor::fromHex(0xBA381F)},
            {"brown",ofColor::fromHex(0x57381F)},
            {"darkbrown",ofColor::fromHex(0x3E2519)},
            {"lightbrown",ofColor::fromHex(0x8E634B)},
            {"orange",ofColor::fromHex(0xBA4B32)},
            {"yellow",ofColor::fromHex(0xC0BA6F)},
            {"green",ofColor::fromHex(0x517525)},
            {"darkgreen",ofColor::fromHex(0x385D12)},
            {"lightgreen",ofColor::fromHex(0x6F8E44)},
            {"blue",ofColor::fromHex(0x5D6FA7)},
            {"lightblue",ofColor::fromHex(0x8EA7A7)},
            {"darkblue",ofColor::fromHex(0x4B575D)},
            {"purple",ofColor::fromHex(0x3E3E44)},
            {"pink",ofColor::fromHex(0xBA381F)}};
    } else if(paletteStr == "amiga") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x000000)},
            {"white",ofColor::fromHex(0xFFFFFF)},
            {"grey",ofColor::fromHex(0xBBBBBB)},
            {"darkgrey",ofColor::fromHex(0x333333)},
            {"lightgrey",ofColor::fromHex(0xFFEEDD)},
            {"gray",ofColor::fromHex(0xBBBBBB)},
            {"darkgray",ofColor::fromHex(0x333333)},
            {"lightgray",ofColor::fromHex(0xFFEEDD)},
            {"red",ofColor::fromHex(0xDD1111)},
            {"darkred",ofColor::fromHex(0x990000)},
            {"lightred",ofColor::fromHex(0xFF4422)},
            {"brown",ofColor::fromHex(0x663311)},
            {"darkbrown",ofColor::fromHex(0x331100)},
            {"lightbrown",ofColor::fromHex(0xAA6644)},
            {"orange",ofColor::fromHex(0xFF6644)},
            {"yellow",ofColor::fromHex(0xFFDD66)},
            {"green",ofColor::fromHex(0x448811)},
            {"darkgreen",ofColor::fromHex(0x335500)},
            {"lightgreen",ofColor::fromHex(0x88BB77)},
            {"blue",ofColor::fromHex(0x8899DD)},
            {"lightblue",ofColor::fromHex(0xBBDDEE)},
            {"darkblue",ofColor::fromHex(0x666688)},
            {"purple",ofColor::fromHex(0x665555)},
            {"pink",ofColor::fromHex(0x997788)}
        };
    } else if(paletteStr == "arnecolors") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x000000)},
            {"white",ofColor::fromHex(0xFFFFFF)},
            {"grey",ofColor::fromHex(0x9d9d9d)},
            {"darkgrey",ofColor::fromHex(0x697175)},
            {"lightgrey",ofColor::fromHex(0xcccccc)},
            {"gray",ofColor::fromHex(0x9d9d9d)},
            {"darkgray",ofColor::fromHex(0x697175)},
            {"lightgray",ofColor::fromHex(0xcccccc)},
            {"red",ofColor::fromHex(0xbe2633)},
            {"darkred",ofColor::fromHex(0x732930)},
            {"lightred",ofColor::fromHex(0xe06f8b)},
            {"brown",ofColor::fromHex(0xa46422)},
            {"darkbrown",ofColor::fromHex(0x493c2b)},
            {"lightbrown",ofColor::fromHex(0xeeb62f)},
            {"orange",ofColor::fromHex(0xeb8931)},
            {"yellow",ofColor::fromHex(0xf7e26b)},
            {"green",ofColor::fromHex(0x44891a)},
            {"darkgreen",ofColor::fromHex(0x2f484e)},
            {"lightgreen",ofColor::fromHex(0xa3ce27)},
            {"blue",ofColor::fromHex(0x1d57f7)},
            {"lightblue",ofColor::fromHex(0xB2DCEF)},
            {"darkblue",ofColor::fromHex(0x1B2632)},
            {"purple",ofColor::fromHex(0x342a97)},
            {"pink",ofColor::fromHex(0xde65e2)}
        };
    } else if(paletteStr == "famicom") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x000000)},
            {"white",ofColor::fromHex(0xffffff)},
            {"grey",ofColor::fromHex(0x7c7c7c)},
            {"darkgrey",ofColor::fromHex(0x080808)},
            {"lightgrey",ofColor::fromHex(0xbcbcbc)},
            {"gray",ofColor::fromHex(0x7c7c7c)},
            {"darkgray",ofColor::fromHex(0x080808)},
            {"lightgray",ofColor::fromHex(0xbcbcbc)},
            {"red",ofColor::fromHex(0xf83800)},
            {"darkred",ofColor::fromHex(0x881400)},
            {"lightred",ofColor::fromHex(0xf87858)},
            {"brown",ofColor::fromHex(0xAC7C00)},
            {"darkbrown",ofColor::fromHex(0x503000)},
            {"lightbrown",ofColor::fromHex(0xFCE0A8)},
            {"orange",ofColor::fromHex(0xFCA044)},
            {"yellow",ofColor::fromHex(0xF8B800)},
            {"green",ofColor::fromHex(0x00B800)},
            {"darkgreen",ofColor::fromHex(0x005800)},
            {"lightgreen",ofColor::fromHex(0xB8F8B8)},
            {"blue",ofColor::fromHex(0x0058F8)},
            {"lightblue",ofColor::fromHex(0x3CBCFC)},
            {"darkblue",ofColor::fromHex(0x0000BC)},
            {"purple",ofColor::fromHex(0x6644FC)},
            {"pink",ofColor::fromHex(0xF878F8)}
        };
    } else if(paletteStr == "atari") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x000000)},
            {"white",ofColor::fromHex(0xFFFFFF)},
            {"grey",ofColor::fromHex(0x909090)},
            {"darkgrey",ofColor::fromHex(0x404040)},
            {"lightgrey",ofColor::fromHex(0xb0b0b0)},
            {"gray",ofColor::fromHex(0x909090)},
            {"darkgray",ofColor::fromHex(0x404040)},
            {"lightgray",ofColor::fromHex(0xb0b0b0)},
            {"red",ofColor::fromHex(0xA03C50)},
            {"darkred",ofColor::fromHex(0x700014)},
            {"lightred",ofColor::fromHex(0xDC849C)},
            {"brown",ofColor::fromHex(0x805020)},
            {"darkbrown",ofColor::fromHex(0x703400)},
            {"lightbrown",ofColor::fromHex(0xCB9870)},
            {"orange",ofColor::fromHex(0xCCAC70)},
            {"yellow",ofColor::fromHex(0xECD09C)},
            {"green",ofColor::fromHex(0x58B06C)},
            {"darkgreen",ofColor::fromHex(0x006414)},
            {"lightgreen",ofColor::fromHex(0x70C484)},
            {"blue",ofColor::fromHex(0x1C3C88)},
            {"lightblue",ofColor::fromHex(0x6888C8)},
            {"darkblue",ofColor::fromHex(0x000088)},
            {"purple",ofColor::fromHex(0x3C0080)},
            {"pink",ofColor::fromHex(0xB484DC)}
        };
    } else if(paletteStr == "pastel") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x000000)},
            {"white",ofColor::fromHex(0xFFFFFF)},
            {"grey",ofColor::fromHex(0x3e3e3e)},
            {"darkgrey",ofColor::fromHex(0x313131)},
            {"lightgrey",ofColor::fromHex(0x9cbcbc)},
            {"gray",ofColor::fromHex(0x3e3e3e)},
            {"darkgray",ofColor::fromHex(0x313131)},
            {"lightgray",ofColor::fromHex(0x9cbcbc)},
            {"red",ofColor::fromHex(0xf56ca2)},
            {"darkred",ofColor::fromHex(0xa63577)},
            {"lightred",ofColor::fromHex(0xffa9cf)},
            {"brown",ofColor::fromHex(0xb58c53)},
            {"darkbrown",ofColor::fromHex(0x787562)},
            {"lightbrown",ofColor::fromHex(0xB58C53)},
            {"orange",ofColor::fromHex(0xEB792D)},
            {"yellow",ofColor::fromHex(0xFFe15F)},
            {"green",ofColor::fromHex(0x00FF4F)},
            {"darkgreen",ofColor::fromHex(0x2b732c)},
            {"lightgreen",ofColor::fromHex(0x97c04f)},
            {"blue",ofColor::fromHex(0x0f88d3)},
            {"lightblue",ofColor::fromHex(0x00fffe)},
            {"darkblue",ofColor::fromHex(0x293a7b)},
            {"purple",ofColor::fromHex(0xff6554)},
            {"pink",ofColor::fromHex(0xeb792d)}
        };
    } else if(paletteStr == "ega") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x000000)},
            {"white",ofColor::fromHex(0xffffff)},
            {"grey",ofColor::fromHex(0x555555)},
            {"darkgrey",ofColor::fromHex(0x555555)},
            {"lightgrey",ofColor::fromHex(0xaaaaaa)},
            {"gray",ofColor::fromHex(0x555555)},
            {"darkgray",ofColor::fromHex(0x555555)},
            {"lightgray",ofColor::fromHex(0xaaaaaa)},
            {"red",ofColor::fromHex(0xff5555)},
            {"darkred",ofColor::fromHex(0xaa0000)},
            {"lightred",ofColor::fromHex(0xff55ff)},
            {"brown",ofColor::fromHex(0xaa5500)},
            {"darkbrown",ofColor::fromHex(0xaa5500)},
            {"lightbrown",ofColor::fromHex(0xffff55)},
            {"orange",ofColor::fromHex(0xff5555)},
            {"yellow",ofColor::fromHex(0xffff55)},
            {"green",ofColor::fromHex(0x00aa00)},
            {"darkgreen",ofColor::fromHex(0x00aaaa)},
            {"lightgreen",ofColor::fromHex(0x55ff55)},
            {"blue",ofColor::fromHex(0x5555ff)},
            {"lightblue",ofColor::fromHex(0x55ffff)},
            {"darkblue",ofColor::fromHex(0x0000aa)},
            {"purple",ofColor::fromHex(0xaa00aa)},
            {"pink",ofColor::fromHex(0xff55ff)}
        };
    } else if(paletteStr == "proteus_mellow") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x3d2d2e)},
            {"white",ofColor::fromHex(0xddf1fc)},
            {"grey",ofColor::fromHex(0x9fb2d4)},
            {"darkgrey",ofColor::fromHex(0x7b8272)},
            {"lightgrey",ofColor::fromHex(0xa4bfda)},
            {"gray",ofColor::fromHex(0x9fb2d4)},
            {"darkgray",ofColor::fromHex(0x7b8272)},
            {"lightgray",ofColor::fromHex(0xa4bfda)},
            {"red",ofColor::fromHex(0x9d5443)},
            {"darkred",ofColor::fromHex(0x8c5b4a)},
            {"lightred",ofColor::fromHex(0x94614c)},
            {"brown",ofColor::fromHex(0x89a78d)},
            {"darkbrown",ofColor::fromHex(0x829e88)},
            {"lightbrown",ofColor::fromHex(0xaaae97)},
            {"orange",ofColor::fromHex(0xd1ba86)},
            {"yellow",ofColor::fromHex(0xd6cda2)},
            {"green",ofColor::fromHex(0x75ac8d)},
            {"darkgreen",ofColor::fromHex(0x8fa67f)},
            {"lightgreen",ofColor::fromHex(0x8eb682)},
            {"blue",ofColor::fromHex(0x88a3ce)},
            {"lightblue",ofColor::fromHex(0xa5adb0)},
            {"darkblue",ofColor::fromHex(0x5c6b8c)},
            {"purple",ofColor::fromHex(0xd39fac)},
            {"pink",ofColor::fromHex(0xc8ac9e)}
        };
    } else if(paletteStr == "proteus_night") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x010912)},
            {"white",ofColor::fromHex(0xfdeeec)},
            {"grey",ofColor::fromHex(0x051d40)},
            {"darkgrey",ofColor::fromHex(0x091842)},
            {"lightgrey",ofColor::fromHex(0x062151)},
            {"gray",ofColor::fromHex(0x051d40)},
            {"darkgray",ofColor::fromHex(0x091842)},
            {"lightgray",ofColor::fromHex(0x062151)},
            {"red",ofColor::fromHex(0xad4576)},
            {"darkred",ofColor::fromHex(0x934765)},
            {"lightred",ofColor::fromHex(0xab6290)},
            {"brown",ofColor::fromHex(0x61646b)},
            {"darkbrown",ofColor::fromHex(0x3d2d2d)},
            {"lightbrown",ofColor::fromHex(0x8393a0)},
            {"orange",ofColor::fromHex(0x0a2227)},
            {"yellow",ofColor::fromHex(0x0a2541)},
            {"green",ofColor::fromHex(0x75ac8d)},
            {"darkgreen",ofColor::fromHex(0x0a2434)},
            {"lightgreen",ofColor::fromHex(0x061f2e)},
            {"blue",ofColor::fromHex(0x0b2c79)},
            {"lightblue",ofColor::fromHex(0x809ccb)},
            {"darkblue",ofColor::fromHex(0x08153b)},
            {"purple",ofColor::fromHex(0x666a87)},
            {"pink",ofColor::fromHex(0x754b4d)}
        };
    } else if(paletteStr == "proteus_rich") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x6f686f)},
            {"white",ofColor::fromHex(0xd1b1e2)},
            {"grey",ofColor::fromHex(0xb9aac1)},
            {"grey",ofColor::fromHex(0x8e8b84)},
            {"tgrey",ofColor::fromHex(0xc7b5cd)},
            {"gray",ofColor::fromHex(0xb9aac1)},
            {"gray",ofColor::fromHex(0x8e8b84)},
            {"tgray",ofColor::fromHex(0xc7b5cd)},
            {"red",ofColor::fromHex(0xa11f4f)},
            {"red",ofColor::fromHex(0x934765)},
            {"tred",ofColor::fromHex(0xc998ad)},
            {"brown",ofColor::fromHex(0x89867d)},
            {"brown",ofColor::fromHex(0x797f75)},
            {"tbrown",ofColor::fromHex(0xab9997)},
            {"ge",ofColor::fromHex(0xce8c5c)},
            {"ow",ofColor::fromHex(0xf0d959)},
            {"green",ofColor::fromHex(0x75bc54)},
            {"green",ofColor::fromHex(0x599d79)},
            {"tgreen",ofColor::fromHex(0x90cf5c)},
            {"blue",ofColor::fromHex(0x8fd0ec)},
            {"tblue",ofColor::fromHex(0xbcdce7)},
            {"blue",ofColor::fromHex(0x0b2c70)},
            {"le",ofColor::fromHex(0x9b377f)},
            {"pink",ofColor::fromHex(0xcd88e5)}
        };
    } else if(paletteStr == "amstrad") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x000000)},
            {"white",ofColor::fromHex(0xffffff)},
            {"grey",ofColor::fromHex(0x7f7f7f)},
            {"darkgrey",ofColor::fromHex(0x636363)},
            {"lightgrey",ofColor::fromHex(0xafafaf)},
            {"gray",ofColor::fromHex(0x7f7f7f)},
            {"darkgray",ofColor::fromHex(0x636363)},
            {"lightgray",ofColor::fromHex(0xafafaf)},
            {"red",ofColor::fromHex(0xff0000)},
            {"darkred",ofColor::fromHex(0x7f0000)},
            {"lightred",ofColor::fromHex(0xff7f7f)},
            {"brown",ofColor::fromHex(0xff7f00)},
            {"darkbrown",ofColor::fromHex(0x7f7f00)},
            {"lightbrown",ofColor::fromHex(0xffff00)},
            {"orange",ofColor::fromHex(0xff007f)},
            {"yellow",ofColor::fromHex(0xffff7f)},
            {"green",ofColor::fromHex(0x01ff00)},
            {"darkgreen",ofColor::fromHex(0x007f00)},
            {"lightgreen",ofColor::fromHex(0x7fff7f)},
            {"blue",ofColor::fromHex(0x0000ff)},
            {"lightblue",ofColor::fromHex(0x7f7fff)},
            {"darkblue",ofColor::fromHex(0x00007f)},
            {"purple",ofColor::fromHex(0x7f007f)},
            {"pink",ofColor::fromHex(0xff7fff)}
        };
    } else if(paletteStr == "c64") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x000000)},
            {"white",ofColor::fromHex(0xffffff)},
            {"grey",ofColor::fromHex(0x6C6C6C)},
            {"darkgrey",ofColor::fromHex(0x444444)},
            {"lightgrey",ofColor::fromHex(0x959595)},
            {"gray",ofColor::fromHex(0x6C6C6C)},
            {"darkgray",ofColor::fromHex(0x444444)},
            {"lightgray",ofColor::fromHex(0x959595)},
            {"red",ofColor::fromHex(0x68372B)},
            {"darkred",ofColor::fromHex(0x3f1e17)},
            {"lightred",ofColor::fromHex(0x9A6759)},
            {"brown",ofColor::fromHex(0x433900)},
            {"darkbrown",ofColor::fromHex(0x221c02)},
            {"lightbrown",ofColor::fromHex(0x6d5c0d)},
            {"orange",ofColor::fromHex(0x6F4F25)},
            {"yellow",ofColor::fromHex(0xB8C76F)},
            {"green",ofColor::fromHex(0x588D43)},
            {"darkgreen",ofColor::fromHex(0x345129)},
            {"lightgreen",ofColor::fromHex(0x9AD284)},
            {"blue",ofColor::fromHex(0x6C5EB5)},
            {"lightblue",ofColor::fromHex(0x70A4B2)},
            {"darkblue",ofColor::fromHex(0x352879)},
            {"purple",ofColor::fromHex(0x6F3D86)},
            {"pink",ofColor::fromHex(0xb044ac)}
        };
    } else if(paletteStr == "whitingjp") {
        palette = {
            {"transparent",ofColor::fromHex(0x0000FF,0)},
            {"black",ofColor::fromHex(0x202527)},
            {"white",ofColor::fromHex(0xeff8fd)},
            {"grey",ofColor::fromHex(0x7b7680)},
            {"darkgrey",ofColor::fromHex(0x3c3b44)},
            {"lightgrey",ofColor::fromHex(0xbed0d7)},
            {"gray",ofColor::fromHex(0x7b7680)},
            {"darkgray",ofColor::fromHex(0x3c3b44)},
            {"lightgray",ofColor::fromHex(0xbed0d7)},
            {"red",ofColor::fromHex(0xbd194b)},
            {"darkred",ofColor::fromHex(0x6b1334)},
            {"lightred",ofColor::fromHex(0xef2358)},
            {"brown",ofColor::fromHex(0xb52e1c)},
            {"darkbrown",ofColor::fromHex(0x681c12)},
            {"lightbrown",ofColor::fromHex(0xe87b45)},
            {"orange",ofColor::fromHex(0xff8c10)},
            {"yellow",ofColor::fromHex(0xfbd524)},
            {"green",ofColor::fromHex(0x36bc3c)},
            {"darkgreen",ofColor::fromHex(0x317610)},
            {"lightgreen",ofColor::fromHex(0x8ce062)},
            {"blue",ofColor::fromHex(0x3f62c6)},
            {"lightblue",ofColor::fromHex(0x57bbe0)},
            {"darkblue",ofColor::fromHex(0x2c2fa0)},
            {"purple",ofColor::fromHex(0x7037d9)},
            {"pink",ofColor::fromHex(0xec2b8f)}
        };
    } else {
        return false;
    }
    
    return true;
    
    
    /*
     1 : "mastersystem",
     2 : "gameboycolour",
     3 : "amiga",
     4 : "arnecolors",
     5 : "famicom",
     6 : "atari",
     7 : "pastel",
     8 : "ega",
     9 : "amstrad",
     10 : "proteus_mellow",
     11 : "proteus_rich",
     12 : "proteus_night",
     13 : "c64",
     14 : "whitingjp"*/
}

void switchToDefaultPalette() {
    switchToPalette("arnecolors");
}
#endif
