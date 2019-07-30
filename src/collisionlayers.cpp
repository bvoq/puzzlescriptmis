#include "collisionlayers.h"

#include "game.h"
#include "logError.h"
#include "objects.h"
#include "stringUtilities.h"


bool parseCollisionLayers(vector<string> lines, int lineFrom, int lineTo, Game & game, Logger & logger) {
    int layerNo = 0;
    
    for(int line=lineFrom;line<lineTo;line++) {
        string str = formatString(lines[line]);
        if(str.size() == 0) continue; // skip empty lines
        str = replaceStrOnce(str, ","," , ");
        vector<string> tokens = tokenize(str);
        
        for(int i=0;i<tokens.size();++i) {
            string token = tokens[i];
            if(tokens[i] == ",") continue; //turns out are optional in puzzlescript
            else {
                if(game.synonyms.count(tokens[i]) != 0) {
                    int index = game.synonyms[ tokens[i] ];
                    if(game.objLayer[ index ] != -1 && game.objLayer[ index ] != layerNo) {
                        logger.logWarning("Defined collision layer of '"+tokens[i]+"' more than once. You should fix this!", line);
                    }
                    game.objLayer[ index ] = layerNo;
                } else if(game.aggregates.count(tokens[i]) != 0) {
                    logger.logError("'"+tokens[i]+"' is an aggregate (defined using 'and'), and cannot be added to a single layer because its constituent objects must be able to coexist.", line);
                    return false;
                } else if(game.properties.count(tokens[i]) != 0) {
                    for(short index : game.properties[ tokens[i] ]) {
                        if(game.objLayer[ index ] != -1 && game.objLayer[ index ] != layerNo) {
                            logger.logWarning("Defined collision layer of '"+game.objPrimaryName[ index ]+"' more than once when defining property '"+tokens[i]+"' (namely "+to_string(game.objLayer[ index ]) +","+to_string(layerNo)+"), you should fix this!", line);
                            //continue;
                            //return false;
                        }
                        game.objLayer[ index ] = layerNo;
                    }
                }
                else {
                        logger.logError("Unknown keyword "+tokens[i]+" in collision layers.", line);
                        return false;
                }
            }
        }
        
        layerNo++;
        game.layerCount = layerNo;
    }
    
    //Now check whether all aggregate blocks are on different levels
    for(auto it = game.aggregates.begin(); it != game.aggregates.end(); it++ )
    {
        set<int> layers;
        for(int index : it->second) {
            if(layers.count( game.objLayer[index] ) != 0) {
                logger.logError("The aggregate '"+(it->first)+"' contains two objects which are on the same layer and can thus not co-exist.",lineFrom-1);
                return false;
            }
            layers.insert(game.objLayer[index]);
        }
        layers.clear();
    }
    
    return true;
}
