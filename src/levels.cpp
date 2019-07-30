#include "levels.h"

#include "game.h"
#include "logError.h"
#include "stringUtilities.h"

#include "utf8/core.h"

#include "utf8/checked.h"
#include "utf8/unchecked.h"

bool parseLevels(vector<string> lines, int lineFrom, int lineTo, Game & game, Logger & logger) { //returns true if successful
    
    int levelCount = 0;
    int currentNumbCols = -1;
    vvvs currentLevel;
    
    for(int line=lineFrom;line<lineTo+1;++line) {
        string str = "";
        if(line < lineTo) str = formatString(lines[line]); //includes lower-casing
        if((str.size()>=7&&str.substr(0,7) == "message") || str.size() == 0) {
            if(currentNumbCols != -1) {
                levelCount++;
                //push back old level
                assert(currentLevel.size() != 0);
                game.levels.push_back(currentLevel);
                
                currentNumbCols = -1;
            }
            
            if(str.size()>=7&&str.substr(0,7) == "message") {
                if(str.size() == 7) game.messages[levelCount].push_back("");
                else game.messages[levelCount].push_back( str.substr(7+1) );
            }
        }
        else {
            if(currentNumbCols == -1) {
                currentNumbCols = actualStringDistance(str);
                currentLevel.clear();
                currentLevel = vvvs (game.layerCount); //initialize empty layers
            }
            if(currentNumbCols == actualStringDistance(str)) {
                vector<string> mapTokens = splitIntoCharsOfSize1(str);
                vector<vector<short> > rowOfTokens (game.layerCount, vector<short> (currentNumbCols));
                int colIndex = 0;
                for(string mapEl : mapTokens) {
                    vector<int> objIndicesToBePlaced = { game.synonyms["background"] };
                    if(game.properties.count(mapEl) != 0) {
                        logger.logError("Cannot use properties in level maps!", line);
                        return false;
                    }
                    else if(game.synonyms.count(mapEl) != 0) {
                        objIndicesToBePlaced.push_back(game.synonyms[mapEl]);
                    } else if(game.aggregates.count(mapEl) != 0) {
                        for(int objIndex : game.aggregates[mapEl]) {
                            objIndicesToBePlaced.push_back(objIndex);
                        }
                    } else {
                        logger.logError("Unknown character "+mapEl+" used when creating level "+to_string(levelCount+1)+".", line);
                        return false;
                    }
                    
                    for(int objIndex : objIndicesToBePlaced) {
                        int objLayer = game.objLayer[objIndex];
                        rowOfTokens[objLayer][colIndex] = objIndex;
                    }
                    colIndex++;
                }
                
                for(int i=0;i<game.layerCount;++i)
                    currentLevel[i].push_back(rowOfTokens[i]);
                
            } else {
                logger.logError("Expected the map to be of square dimension (expected "+to_string(currentNumbCols)+" but received " +to_string(actualStringDistance(str))+".",line);
                return false;
            }
        }
    }
    
    for(int i=0;i<game.objLayer.size();++i) {
        cout << i << ": " << game.objLayer[i] << " " << game.objPrimaryName[i] << endl;
    }
    
    // Update player indices and player layers for ease of access.
    game.playerIndices.clear();
    if(game.synonyms.count("player") != 0) {
        game.playerIndices = { { game.synonyms["player"], game.objLayer[game.synonyms["player"]] } };
    } else if(game.properties.count("player") != 0) {
        for(short objIndex : game.properties["player"]) {
            game.playerIndices.push_back( {objIndex, game.objLayer[objIndex]});
        }
    } else if(game.aggregates.count("player") != 0) {
        for(short objIndex : game.aggregates["player"]) {
            game.playerIndices.push_back( {objIndex, game.objLayer[objIndex]});
        }
    }
        
    if(game.levels.size() == 0) {
        logger.logError("Expected at least one level.",lineTo-1);
        return false;
    }
    
    return true;
}


//changes the level string to the new level. if a new block with no single character string is added, it will generate the single character string.
void changeLevelStr(vector<string> & levelstr, int level, vvvs newLevel, vvvs oldLevel, const Game & oldgame) {
    assert(oldgame.synonyms.at("background") != 0);
    map<string,vector<short> > singleCharStrToAgg;
    map<vector<short>, string > aggToSingleCharStr;
    for(auto p : oldgame.aggsWithSingleCharName) {
        vector<short> newS = p.second;
        if(find(newS.begin(),newS.end(), oldgame.synonyms.at("background")) == newS.end())
            newS.push_back(oldgame.synonyms.at("background"));
        sort(newS.begin(), newS.end());
        singleCharStrToAgg[p.first] = newS;
        aggToSingleCharStr[newS] = p.first;
    }
    for(auto p : oldgame.synsWithSingleCharName) {
        vector<short> newS = {p.second};
        if(p.second != oldgame.synonyms.at("background")) newS.push_back(oldgame.synonyms.at("background"));

        sort(newS.begin(), newS.end());
        singleCharStrToAgg[p.first] = newS;
        aggToSingleCharStr[newS] = p.first;
    }
    
    vector<vector<short> > newSingleCharAggs; //when creating a new level there is a chance it contains a tile that does not have a single character inside the tiling.
    
    vector<string> copied = levelstr; //exactly like levelstr but comments are removed.
    int commentLevel = 0;
    for(int i=0;i<copied.size();++i) {
        for(int j=0;j<copied[i].size();++j) {
            if(copied[i][j] == '(') {
                commentLevel++;
                copied[i][j] = ' ';
            }
            else if(copied[i][j] == ')' && commentLevel > 0) {
                commentLevel--;
                copied[i][j] = ' ';
            }
            else if(commentLevel > 0) copied[i][j] = ' ';
        }
    }
    
    int levelCount = 0;
    int currentNumbCols = -1;
    vvvs currentLevel;
    int currentRow = 0;
    
    int lineFrom = levelstr.size();
    for(int q=0;q<levelstr.size();++q) {
        if(formatString(copied[q]) == "levels") lineFrom = q+1;
    }
    for(int line=lineFrom;line<levelstr.size();++line) {
        string str = "";
        if(line < levelstr.size()) str = formatString(copied[line]); //includes lower-casing
        if((str.size()>=7&&str.substr(0,7) == "message") || str.size() == 0) {
            if(currentNumbCols != -1) {
                levelCount++;
                currentNumbCols = -1;
                currentRow = 0;
            }
        }
        else {
            if(currentNumbCols == -1) {
                currentNumbCols = actualStringDistance(str);
                currentRow = 0;
            }
            if(currentNumbCols == actualStringDistance(str)) { //still in the same level
                if(levelCount == level) {
                    
                    for(int x=0;x<newLevel[0][currentRow].size();++x) {
                        bool equal = true;
                        for(int l=0;l<newLevel.size();++l) {
                            //if not the same
                            if(newLevel[l][currentRow][x] != oldLevel[l][currentRow][x]) equal = false;
                        }
                        if(!equal) {
                            //find appropriate single char symbol or create it if it doesn't exist
                            equal = false;
                            vector<short> currentAgg;
                            for(int l=0;l<newLevel.size();++l) {
                                if(newLevel[l][currentRow][x] != 0) currentAgg.push_back(newLevel[l][currentRow][x]);
                            }
                            sort(currentAgg.begin(), currentAgg.end());
                            
                            
                            
                            if(aggToSingleCharStr.count(currentAgg) == 0) { //create new agg
                                //first go through 0-9, then a-z, then any remaining single character in the ASCII
                                static vector<string> possibleUnusedSingleCharStrs = {
                                    "0","1","2","3","4","5","6","7","8","9",
                                    "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z",
                                    "@","*","\"","-","$","£"
                                };
                                for(string possibleUnusedStr : possibleUnusedSingleCharStrs) {
                                    if(singleCharStrToAgg.count(possibleUnusedStr) == 0) {
                                        newSingleCharAggs.push_back(currentAgg);
                                        singleCharStrToAgg[possibleUnusedStr] = currentAgg;
                                        aggToSingleCharStr[currentAgg] = possibleUnusedStr;
                                        break;
                                    }
                                }
                            }
                            
                            string c = "";
                            c = aggToSingleCharStr[currentAgg];
                            levelstr[line] = setString(levelstr[line], x, c);
                        }
                    }
                }
                currentRow++;
            }
        }
    }
    
    
    //ADD NEW SINGLE CHAR SYMBOLS
    vector<string> finallines;
    bool addedNewAggs = false;
    for(int q=0;q<levelstr.size();++q) {
        if(!addedNewAggs) {
            int kq=q;
            while(kq < levelstr.size() && formatString(copied[kq]).size() == 0) kq++;
            if(formatString(copied[kq]) == "sounds" || formatString(copied[kq]) == "collisionlayers") { //whichever section happens first
                for(int i=0;i<newSingleCharAggs.size();++i) {
                    string newStr = aggToSingleCharStr[newSingleCharAggs[i]] + " = ";
                    for(int j=0;j<newSingleCharAggs[i].size();++j) {
                        if(oldgame.synonyms.at("background") != newSingleCharAggs[i][j])
                            newStr += oldgame.objPrimaryName[newSingleCharAggs[i][j]] + (j+1!=newSingleCharAggs[i].size() ? " and " : "");
                    }
                    finallines.push_back(newStr);
                }
                addedNewAggs = true;
            }
        }
        finallines.push_back(levelstr[q]);
    }
    
    levelstr = finallines;    
}
