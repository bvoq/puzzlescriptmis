#include "winconditions.h"

#include "game.h"
#include "logError.h"
#include "stringUtilities.h"


bool parseWinconditions(vector<string> lines, int lineFrom, int lineTo, Game & game, Logger & logger) { //returns true if successful
    for(int line=lineFrom;line<lineTo;++line) {
        string str = formatString(lines[line]); //includes lower-casing
        if(str.size() == 0) continue; // skip empty lines
        str = replaceStrOnce(str, "="," = ");
        vector<string> tokens = tokenize(str);
        
        if(tokens.size() <= 1 || tokens.size() == 3 || tokens.size() >= 5) {
            logger.logError("Wrong wincondition format.",line);
            return false;
        }
        
        if(tokens.size() == 2 && (tokens[0] == "no" || tokens[0] == "some" || tokens[0] == "any")) {
            if(game.synonyms.count(tokens[1]) != 0) {
                short objIndex = game.synonyms[tokens[1]];
                short objLayer = game.objLayer[objIndex];
                if(objLayer == -1) {
                    logger.logError("Token '"+tokens[1]+"' has to have a collisionlayer assigned before using it in a wincondition." ,line);
                    return false;
                }
                cerr << "marr " << objIndex << " " << objLayer << endl;
                if(tokens[0] == "no")
                    game.winConditionsNo.push_back({objIndex,objLayer});
                else if(tokens[0] == "some" || tokens[0] == "any")
                    game.winConditionsSome.push_back({{objIndex,objLayer}});
            } else if(game.properties.count(tokens[1]) != 0) {
                if(tokens[0] == "some") game.winConditionsSome.push_back({});
                cerr << "WINCOND " << tokens[0] << " " << tokens[1] << endl;
                for(short objIndex : game.properties.at( tokens[1] )) {
                    short objLayer = game.objLayer[objIndex];
                    if(objLayer == -1) {
                        logger.logError("Token '"+game.objPrimaryName.at(objIndex) +" has to have a collisionlayer assigned before using it in a wincondition." ,line);
                        return false;
                    }
                    if(tokens[0] == "no")
                        game.winConditionsNo.push_back({objIndex,objLayer});
                    else if(tokens[0] == "some")
                        game.winConditionsSome.back().push_back({objIndex,objLayer});
                }
            } else {
                logger.logError("Unwelcome term '"+tokens[1]+"' found in win condition. Win conditions objects have to be objects or properties (defined using 'or', in terms of other properties/objects)",line);
                return false;
            }
        }
        
        
        
        else if(tokens.size() == 4 && (tokens[0] == "no" || tokens[0] == "some" || tokens[0] == "all")) {
            if(tokens[2] != "on") {
                logger.logError("Expected 'on' token in wincondition.",line);
                return false;
            }
            //LHS
            if(game.synonyms.count(tokens[1]) != 0) {
                short objIndex = game.synonyms[tokens[1]];
                short objLayer = game.objLayer[objIndex];
                if(objLayer == -1) {
                    logger.logError("Token '"+tokens[1]+"' has to have a collisionlayer assigned before using it in a wincondition." ,line);
                    return false;
                }
                if(tokens[0] == "no")
                    game.winConditionsNoXOnYLHS.push_back({{objIndex,objLayer}});
                else if(tokens[0] == "some")
                    game.winConditionsSomeXOnYLHS.push_back({{objIndex,objLayer}});
                else if(tokens[0] == "all")
                    game.winConditionsAllXOnYLHS.push_back({{objIndex,objLayer}});
            } else if(game.properties.count(tokens[1]) != 0) {
                if(tokens[0] == "no") game.winConditionsNoXOnYLHS.push_back({});
                else if(tokens[0] == "some") game.winConditionsSomeXOnYLHS.push_back({});
                else if(tokens[0] == "all") game.winConditionsAllXOnYLHS.push_back({});
                for(short objIndex : game.properties[ tokens[1] ]) {
                    short objLayer = game.objLayer[objIndex];
                    if(objLayer == -1) {
                        logger.logError("Token '"+game.objPrimaryName.at(objIndex)+"' has to have a collisionlayer assigned before using it in a wincondition." ,line);
                        return false;
                    }
                    if(tokens[0] == "no")
                        game.winConditionsNoXOnYLHS.back().push_back({objIndex,objLayer});
                    else if(tokens[0] == "some")
                        game.winConditionsSomeXOnYLHS.back().push_back({objIndex,objLayer});
                    else if(tokens[0] == "all")
                        game.winConditionsAllXOnYLHS.back().push_back({objIndex,objLayer});
                }
            } else {
                logger.logError("Unwelcome term '"+tokens[1]+"' found in win condition. Win conditions objects have to be objects or properties (defined using 'or', in terms of other properties/objects)",line);
                return false;
            }
            
            //RHS
            if(game.synonyms.count(tokens[3]) != 0) {
                short objIndex = game.synonyms[tokens[3]];
                short objLayer = game.objLayer[objIndex];
                if(objLayer == -1) {
                    logger.logError("Token '"+tokens[3]+"' has to have a collisionlayer assigned before using it in a wincondition." ,line);
                    return false;
                }
                if(tokens[0] == "no")
                    game.winConditionsNoXOnYRHS.push_back({{objIndex,objLayer}});
                else if(tokens[0] == "some")
                    game.winConditionsSomeXOnYRHS.push_back({{objIndex,objLayer}});
                else if(tokens[0] == "all")
                    game.winConditionsAllXOnYRHS.push_back({{objIndex,objLayer}});
            } else if(game.properties.count(tokens[3]) != 0) {
                if(tokens[0] == "no") game.winConditionsNoXOnYRHS.push_back({});
                else if(tokens[0] == "some") game.winConditionsSomeXOnYRHS.push_back({});
                else if(tokens[0] == "all") game.winConditionsAllXOnYRHS.push_back({});
                for(short objIndex : game.properties[ tokens[3] ]) {
                    short objLayer = game.objLayer[objIndex];
                    if(objLayer == -1) {
                        logger.logError("Token '"+game.objPrimaryName.at(objIndex)+"' has to have a collisionlayer assigned before using it in a wincondition." ,line);
                        return false;
                    }
                    if(tokens[0] == "no")
                        game.winConditionsNoXOnYRHS.back().push_back({objIndex,objLayer});
                    else if(tokens[0] == "some")
                        game.winConditionsSomeXOnYRHS.back().push_back({objIndex,objLayer});
                    else if(tokens[0] == "all")
                        game.winConditionsAllXOnYRHS.back().push_back({objIndex,objLayer});
                }
            } else {
                logger.logError("Unwelcome term '"+tokens[1]+"' found in win condition. Win conditions objects have to be objects or properties (defined using 'or', in terms of other properties/objects)",line);
                return false;
            }
        }
        else {
            logger.logError("Not a valid win-condition.", line);
            return false;
        }
    }
    
    return true;
}

// unwelcome term "playerandtarget" found in win condition. Win conditions objects have to be objects or properties (defined using "or", in terms of other properties)


// All property on X => winConditionsAllOnY

// All object on X
// All aggregate on X => winConditionsAllOnY
