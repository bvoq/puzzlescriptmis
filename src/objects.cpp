#include "objects.h"

#include "colors.h"
#include "game.h"
#include "logError.h"
#include "objects.h"
#include "stringUtilities.h"


int PIXEL_RESOLUTION = 5;

static inline uint64_t hashColor(const ofColor & c, uint64_t t) {
    t = FNV64(c.getHex(), t);
    t = FNV64(c.a, t);
    return t;
}

bool parseObjects(vector<string> lines, int lineFrom, int lineTo, Game & game, Logger & logger) { //returns true if successful
    assert(lineTo < lines.size() && lineFrom < lineTo);
    game.objLayer.clear();
    game.objPrimaryName.clear(); //just to make sure
    game.objLayer.push_back(-1); //intial empty object
    game.objPrimaryName.push_back("no_object");
#ifndef compiledWithoutOpenframeworks
    game.objTexture.clear();
    ofPixels cpixels; //initialize pixels
    uint64_t chash = INITIAL_HASH;
    cpixels.allocate(1, 1, OF_PIXELS_RGBA);
    cpixels.setColor(ofColor(0,0,0,255));
    chash = hashColor(ofColor(0,0,0,255), chash);
    ofTexture temptex;
    temptex.allocate(cpixels);
    temptex.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST); //no aliasing
    if( colors::textureMap.count(chash) != 0) {
        colors::textureMap[chash] = colors::textures.size();
        colors::textures.push_back(temptex);
    }
    game.objTexture.push_back( colors::textureMap[chash] );
    vector<ofColor> ccolors;
    
#endif
    

    
    int parseMode = 0; // 0 = define block, 1 = colors, 2...pixel_resolution-1 = read sprite graphics
    
    bool finalizeObject = false;
    
    short cindex = 0;  //current index of the block being read
    string cname = "";
    for(int line=lineFrom;line<lineTo;++line) {
        string str = formatString(lines[line]);
        if(str.size() == 0) continue; // skip empty lines
        
        if(parseMode == 0) {
            //Initialize current block and make layer -1
            //cout << "errr: " << (int)cindex << endl;
            if(cindex == (short) 255) {
                logger.logError("Only 255 different blocks are supported!", line);
                return false;
            }
            cindex = game.objLayer.size();


#ifndef compiledWithoutOpenframeworks
            //Empty pixel buffer & color array
            cpixels.clear();
            cpixels.allocate(PIXEL_RESOLUTION, PIXEL_RESOLUTION, OF_PIXELS_RGBA);
            ccolors.clear();
            chash = INITIAL_HASH;
#endif
            //Tokenize
            vector<string> tokens = tokenize(str);
            
            //Parse names
            bool isprimaryname = true;
            for(string token : tokens) {
                bool success = game.insertSynonym(token, cindex, line, logger);
                if(!success) {
                    logger.logError("Keyword '"+token+ "' already defined or reserved!",line);
                    return false;
                }
                
                if(isprimaryname) {
                    cname = token;
                    isprimaryname = false;
                }
            }
            
        } else if(parseMode == 1) {
            #ifndef compiledWithoutOpenframeworks
            //Tokenize
            vector<string> tokens = tokenize(str);
            
            //Parse colors

            for(string token : tokens) {
                if(colors::palette.count(token) != 0) {
                    ccolors.push_back( colors::palette[token] );
                } else if(token[0] == '#' && token.size() == 7) {
                    int hexColor = 0;
                    for(int i=1;i<=6;++i) {
                        if(token[i] >= 'A' && token[i] <= 'F') hexColor += token[i]- 'A'+10;
                        else if(token[i] >= 'a' && token[i] <= 'f') hexColor += token[i]- 'a'+10;
                        else if(token[i] >= '0' && token[i] <= '9') hexColor += token[i]-'0';
                        else {
                            logger.logError("Expected color, but received: "+token, line);
                            return false;
                        }
                        if(i<6) hexColor *= 16;
                    }
                                        
                    ofColor newColor;
                    newColor.setHex(hexColor);
                    ccolors.push_back( newColor );
                    cerr << "errr" << std::hex << hexColor << endl;
                } else if(token[0] == '#' && token.size() == 9) {
                    int hexColor = 0;
                    for(int i=1;i<=6;++i) {
                        if(token[i] >= 'A' && token[i] <= 'F') hexColor += token[i]- 'A'+10;
                        else if(token[i] >= 'a' && token[i] <= 'f') hexColor += token[i]- 'a'+10;
                        else if(token[i] >= '0' && token[i] <= '9') hexColor += token[i]-'0';
                        else {
                            logger.logError("Expected color, but received: "+token, line);
                            return false;
                        }
                        if(i<6) hexColor *= 16;
                    }
                    
                    int transp=0;
                    if(token[7] >= 'A' && token[7] <= 'F') transp += token[7]- 'A'+10;
                    else if(token[7] >= 'a' && token[7] <= 'f') transp += token[7]- 'a'+10;
                    else if(token[7] >= '0' && token[7] <= '9') transp += token[7]-'0';
                    transp *= 16;
                    if(token[8] >= 'A' && token[8] <= 'F') transp += token[8]- 'A'+10;
                    else if(token[8] >= 'a' && token[8] <= 'f') transp += token[8]- 'a'+10;
                    else if(token[8] >= '0' && token[8] <= '9') transp += token[8]-'0';
                    
                    ofColor newColor;
                    newColor.setHex(hexColor);
                    newColor.a = transp;
                    ccolors.push_back( newColor );
                    
                    
                } else if(token[0] == '#' && token.size() == 4) { //special triple hex format
                    int hexColor = 0;
                    for(int i=3;i>=1;--i) {
                        if(token[i] >= 'A' && token[i] <= 'F') hexColor += (token[i]- 'A'+10)*16;
                        else if(token[i] >= 'a' && token[i] <= 'f') hexColor += (token[i]- 'a'+10)*16;
                        else if(token[i] >= '0' && token[i] <= '9') hexColor += (token[i]-'0')*16;
                        else {
                            logger.logError("Expected color, but received: "+token, line);
                            return false;
                        }
                        if(i>1) hexColor *= 16*16;
                    }
                    
                    ofColor newColor;
                    newColor.setHex(hexColor);
                    ccolors.push_back( newColor );
                } else {
                    logger.logError("Expected color, but received: "+token, line);
                    return false;
                }
            }
            #endif
        }
        //Case where an object does not have a legend
        else if (parseMode == 2 && ! (str[0] == '.' || (str[0]>='0'&&str[0]<='9') )) { 
            #ifndef compiledWithoutOpenframeworks
            cpixels.setColor( ccolors[0] );
            chash = hashColor(ccolors[0], chash);
            #endif
            
            line--; //repeat the next previous line again
            finalizeObject = true;
        }
        //Case where an object has a legend
        else if(parseMode >= 2 && parseMode <= 1+PIXEL_RESOLUTION) {
            // Read legend
            if(str.size() != PIXEL_RESOLUTION) {
                logger.logError("Sprites have to be "+to_string(PIXEL_RESOLUTION)+"-by-"+to_string(PIXEL_RESOLUTION)+".", line);
                return false;
            }
            int y = parseMode - 2;
            for(int x=0;x<PIXEL_RESOLUTION;++x) {
                if(str[x] == '.') {
                    #ifndef compiledWithoutOpenframeworks
                    cpixels.setColor(x,y,colors::palette["transparent"]);
                    chash = hashColor(colors::palette["transparent"], chash);
                    #endif
                } //transparent
                else if(str[x] >= '0' && str[x] <= '9') {
                    #ifndef compiledWithoutOpenframeworks
                    if( (str[x]-'0') >= ccolors.size() ) {
                        logger.logError("Used sprite index "+to_string(str[x])+" but only defined "+to_string(ccolors.size())+ " colors.", line);
                        return false;
                    }
                    cpixels.setColor(x,y,ccolors[str[x]-'0']);
                    chash = hashColor(ccolors[str[x]-'0'], chash);
                    #endif
                }
                if(parseMode == 1+PIXEL_RESOLUTION) finalizeObject = true;
            }
        }
        
        //Special case if the last line does not have a 5x5 legend and only colors specified:
        if(parseMode == 1) {
            //check if all the following lines are empty
            bool allEmpty = true;
            for(int j=line+1;j<lineTo; ++j) {
                if(formatString(lines[j]).size() > 0) allEmpty = false;
            }
            if(allEmpty) {
                #ifndef compiledWithoutOpenframeworks
                cpixels.setColor( ccolors[0] );
                chash = hashColor(ccolors[0] , chash);
                #endif
                finalizeObject = true;
            }
        }
        
        if(finalizeObject) {
            game.objLayer.push_back(-1);
            game.objPrimaryName.push_back(cname);

            #ifndef compiledWithoutOpenframeworks

            if( colors::textureMap.count(chash) == 0) {
                ofTexture tex;
                tex.allocate(cpixels);
                tex.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST); //no aliasing
                
                colors::textureMap[chash] = colors::textures.size();
                colors::textures.push_back(tex);
            }
            game.objTexture.push_back( colors::textureMap[chash] );
            
            #endif

            finalizeObject = false;
            parseMode = 0;
        } else {
            (++parseMode) %= PIXEL_RESOLUTION+2;
        }
    }
    
    return true;
}
