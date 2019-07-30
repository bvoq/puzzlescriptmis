#include "legend.h"

#include "game.h"
#include "logError.h"
#include "objects.h"
#include "stringUtilities.h"

bool parseLegend(vector<string> lines, int lineFrom, int lineTo, Game & game, Logger & logger) { //returns true if successful
    
    for(int line=lineFrom;line<lineTo;++line) {
        string str = formatString(lines[line]); //includes lower-casing
        if(str.size() == 0) continue; // skip empty lines
        str = replaceStrOnce(str, "="," = ");
        vector<string> tokens = tokenize(str);
        
        if(tokens.size()<3) {
            logger.logError("Something seems to be missing in the legend.",line);
            return false;
        }
        
        if(tokens[1] != "=") {
            logger.logError("Expected '=' symbol in the legend.",line);
            return false;
        }

        if(tokens.size() == 3) { //SYNONYMS of synonyms or aggregates or properties
            if(game.synonyms.count(tokens[2]) == 0 && game.properties.count(tokens[2]) == 0 && game.aggregates.count(tokens[2]) == 0) {
                logger.logError("Unknown object "+tokens[2]+ " when declaring synonym "+tokens[0]+".", line);
                return false;
            }
            if(game.synonyms.count(tokens[2]) != 0)  {
                bool success = game.insertSynonym(tokens[0], game.synonyms[ tokens[2] ], line, logger);
                if(!success) {
                    logger.logError("Keyword "+tokens[0]+ " already defined or reserved!",line);
                    return false;
                }
            } else if(game.aggregates.count(tokens[2]) != 0) {
                bool success = game.insertAggregate(tokens[0], game.aggregates[ tokens[2] ], line, logger);
                if(!success) {
                    logger.logError("Keyword "+tokens[0]+ " already defined or reserved!",line);
                    return false;
                }
            } else if(game.properties.count(tokens[2]) != 0) {
                bool success = game.insertProperty(tokens[0], game.properties[ tokens[2] ], line, logger);
                if(!success) {
                    logger.logError("Keyword "+tokens[0]+ " already defined or reserved!",line);
                    return false;
                }
            }
        } else if(tokens[3] == "and") { //AGGREGATES
            set<short> aggregateIndicesSet;
            for(int i=2;i<tokens.size();++i) {
                if(i%2==1 && tokens[i] != "and") {
                    logger.logError("Expected 'and' in the legend.",line);
                    return false;
                } else if(i%2==0) {
                    if(game.properties.count(tokens[i]) != 0) {
                        logger.logError("Cannot use a property "+tokens[i]+" to define the aggregate "+tokens[0]+".",line);
                        return false;
                    } else if(game.synonyms.count(tokens[i]) == 0 && game.aggregates.count(tokens[i]) == 0) {
                        logger.logError("Unknown keyword "+tokens[i]+" for defining aggregate "+tokens[0]+".",line);
                        return false;
                    } else if(game.synonyms.count(tokens[i]) != 0) {
                        aggregateIndicesSet.insert( game.synonyms[tokens[i]]);
                        //checking whether layers collide in aggregates is handled later in the collisions section
                    } else if(game.aggregates.count(tokens[i]) != 0) {
                        for(int index : game.aggregates[tokens[i]]) {
                            aggregateIndicesSet.insert(index);
                        }
                    }
                }
            }
            
            vector<short> aggregateIndices;
            copy(aggregateIndicesSet.begin(), aggregateIndicesSet.end(), back_inserter(aggregateIndices));
            
            bool success = game.insertAggregate(tokens[0], aggregateIndices, line, logger);
            if(!success) {
                logger.logError("Keyword "+tokens[0]+ " already defined or reserved!",line);
                return false;
            }
        } else if(tokens[3] == "or") { //property
            
            /*WRONG:
             if(tokens[0] == "player") {
                logger.logError("Player cannot be a property.",lines[line],line);
                return false;
            }*/
            
            
            set<short> propertyIndicesSet;
            for(int i=2;i<tokens.size();++i) {
                if(i%2==1 && tokens[i] != "or") {
                    logger.logError("Expected 'or' in the legend.",line);
                    return false;
                } else if(i%2==0) {
                    if(game.aggregates.count(tokens[i]) != 0) {
                        logger.logError("Cannot use the aggregate '"+tokens[i]+"' to define the property '"+tokens[0]+"'.",line);
                        return false;
                    } else if(game.synonyms.count(tokens[i]) == 0 && game.properties.count(tokens[i]) == 0) {
                        logger.logError("Unknown keyword "+tokens[i]+" for defining property "+tokens[0]+".",line);
                        return false;
                    } else if(game.synonyms.count(tokens[i]) != 0) {
                        propertyIndicesSet.insert( game.synonyms[tokens[i]] );
                    } else if(game.properties.count(tokens[i]) != 0) {
                        for(int index : game.properties[tokens[i]]) {
                            propertyIndicesSet.insert(index);
                        }
                    }
                }
            }
            
            vector<short> propertyIndices;
            copy(propertyIndicesSet.begin(), propertyIndicesSet.end(), back_inserter(propertyIndices));
            
            bool success = game.insertProperty(tokens[0], propertyIndices, line, logger);
            if(!success) {
                logger.logError("Keyword "+tokens[0]+ " already defined or reserved!",line);
                return false;
            }
            
        } else {
            logger.logError("Ill-defined legend object (expected 'and' or 'or').", line);
            return false;
        }
        
    }
    
    
    //check player indices
    
    if(game.synonyms.count("player") == 0 && game.properties.count("player") == 0 && game.aggregates.count("player") == 0) {
        logger.logWarning("No player defined.", lineFrom);
    }
    
    
    
    if(game.synonyms.count("background") == 0) {
        
        logger.logError("No background defined.", lineFrom);
        return false;
    }
    
    
    //previously some objects might be single character:
    /*
    set<short> alreadyAdded;
    for(auto const& c: game.synsWithSingleCharName)
        alreadyAdded.insert(c.second);
    for (auto const& x : game.synonyms)
        if(actualStringDistance(x.first) == 1 && alreadyAdded.count(x.second) == 0) {
            alreadyAdded.insert(x.second);
            game.synsWithSingleCharName.push_back({x.first, x.second});
        }
    */
    return true;
}
