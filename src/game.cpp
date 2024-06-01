#include "game.h"

#include "collisionlayers.h"
#include "colors.h"
#include "engine.h"
#include "global.h"
#include "legend.h"
#include "levels.h"
#include "logError.h"
#include "objects.h"
#include "rules.h"
#include "solver.h"
#include "stringUtilities.h"
#include "winconditions.h"

#include <functional>


static set<string> reservedNames = {
    "->","=","|","[","]","(",")","...", //symbols
    ">","<","^","v","up","down","left","right","stationary","no","randomdir","random","perpendicular","parallel","moving","horizontal","vertical","orthogonal","action", // movements
    
    "or","and", //define statements
    
    "option", //special statements for specifying generation constraints
}; //names which can't be redefined (things will be added to this list so the user doesn't redefine things twice)!


bool Game::insertSynonym (string str, short index, int lineno, Logger & logger) {
    if(reservedNames.count(str) != 0) {
        logger.logWarning("You try to define '"+str+"' even though it's a reserved keyword. Don't do that!",0);
    }
    if(definedNames.count(str) != 0) {
        if(synonyms.count(str) != 0 && synonyms[str] == index) {
            logger.logWarning("You initialise the keyword "+str+" twice, don't do that!",lineno);
            return true; //identical double definition OK
        }
        return false;
    }
    if(actualStringDistance(str) == 1) {
        synsWithSingleCharName.push_back({str, index});
    }
    //assert(index < objLayer.size());
    definedNames.insert(str);
    synonyms[str] = index;
    return true;
}

bool Game::insertAggregate (string str, vector<short> indices, int lineno, Logger & logger) {
    if(reservedNames.count(str) != 0) {
        logger.logWarning("You try to define '"+str+"' even though it's a reserved keyword. Don't do that!",lineno);
    }
    if(definedNames.count(str) != 0) {
        if(aggregates.count(str) != 0 && aggregates[str] == indices) return true; //identical double definition OK
        return false;
    }
    if(actualStringDistance(str) == 1) {
        aggsWithSingleCharName.push_back({str, indices});
    }
    definedNames.insert(str);
    aggregates[str] = indices;
    return true;
}

bool Game::insertProperty (string str, vector<short> indices, int lineno, Logger & logger) {
    if(reservedNames.count(str) != 0) {
        logger.logWarning("You try to define '"+str+"' even though it's a reserved keyword. Don't do that!",lineno);
    }
    if(definedNames.count(str) != 0) {
        if(properties.count(str) != 0 && properties[str] == indices) return true; //identical double definition OK
        return false;
    }
    definedNames.insert(str);
    properties[str] = indices;
    return true;
}

void Game::updateLevelState(vvvs newCurrentState, int index) {
    assert(index < levels.size());
    levels[index] = newCurrentState;
    if(index == currentLevelIndex) {
        currentState = newCurrentState;
        assert(currentState.size() != 0 && currentState[0].size() != 0);
        currentLevelHeight = currentState[0].size();
        currentLevelWidth = currentState[0][0].size();

        moveAndChangeField(STATIONARY_MOVE, newCurrentState, *this);
        beginStateAfterStationaryMove = newCurrentState;
    }
    startSolving(0, this->beginStateAfterStationaryMove, *this, gbl::emptyMoves);
}

uint64_t Game::getHash() const { //only includes the hash for the mechanics
    //if(hash != 0) return hash;
    uint64_t chash = INITIAL_HASH;
    
    for(const auto & x : objLayer) {
        chash = FNV64(x, chash);
    }
    for(const auto & x : winConditionsNo) {
        chash = FNV64(x.first, chash);
        chash = FNV64(x.second, chash);
    }
    for(const auto & x : playerIndices) {
        chash = FNV64(x.first, chash);
        chash = FNV64(x.second, chash);
    }
    
    #define HashVVP(q)\
    for(const auto & x : q) { \
        for(const auto & y : x) { \
            chash = FNV64(y.first, chash);\
            chash = FNV64(y.second, chash);\
        }\
    }\

    HashVVP(winConditionsSome);
    HashVVP(winConditionsNoXOnYLHS);
    HashVVP(winConditionsNoXOnYRHS);
    HashVVP(winConditionsSomeXOnYLHS);
    HashVVP(winConditionsSomeXOnYRHS);
    HashVVP(winConditionsAllXOnYLHS);
    HashVVP(winConditionsAllXOnYRHS);

    for(const Rule & rule : rules) {
        chash = rule.getHash(chash);
    }
    return chash;
}

uint64_t Game::getTotalHash() const {
    uint64_t chash = getHash(); //get the hash from the ruleset
    
    HashVVV(currentState,chash);
    HashVVV(beginStateAfterStationaryMove,chash);
    chash = FNV64(currentLevelIndex, chash);
    chash = FNV64(currentLevelHeight, chash);
    chash = FNV64(currentLevelWidth, chash);
    
    for(size_t i=0;i<levels.size();++i) {
        HashVVV(levels[i],chash);
    }

    for (const auto & x : messages) {
        chash = FNV64(x.first, chash);
        for(const auto & y : x.second)
            chash = FNV64(std::hash<std::string>()(y), chash);
    }
    chash = FNV64(currentMessageIndex, chash);

    for(const auto & x : objPrimaryName) {
        chash = FNV64(std::hash<std::string>()(x), chash);
    }
    for(const auto & x : objTexture) {
        chash = FNV64(x, chash);
    }
    
    for(const Rule & rule : generatorRules) {
        chash = rule.getHash(chash);
    }
    
    return chash;
}

inline uint64_t Game::getGameStateHash(const vvvs & state) const {
    uint64_t chash = this->getHash();
    HashVVV(state, chash);
    return chash;
}

deque<short> Game::getCurrentMoves() const {
    deque<short> currentMoves;
    for(int i=(int)this->undoStates.size()-1; i >= 0 && this->undoStates[i].second != (short)0x0101010; --i) { //until restart key or no more moves done
        currentMoves.push_front(this->undoStates[i].second);
    }
    return currentMoves;
}


ostream& operator<<(ostream& os, const Game & g) {
    os << g.currentState << endl; // this contains the entire level structure and is simply a deque<deque<deque<int> > >
    os << g.beginStateAfterStationaryMove << endl;
    os << g.undoStates << endl;
    os << g.currentLevelIndex << endl;
    os << g.currentLevelHeight << endl << g.currentLevelWidth << endl;
    os << g.levels << endl;
    //these are explicity left empty!
    //os << g.messages << endl;
    //os << g.currentMessageIndex << endl;
    os << g.objPrimaryName << endl;
    os << g.objTexture << endl;
    os << g.objLayer << endl;
    os << g.layerCount << endl;
    os << g.rules << endl;
    os << g.synonyms << endl; //only those with utf8 length 1 can be used when drawing a map!
    os << g.aggregates << endl;
    os << g.properties << endl;
    os << g.definedNames << endl;
    os << g.playerIndices << endl;
    os << g.synsWithSingleCharName << endl;
    os << g.aggsWithSingleCharName << endl;
    os << g.winConditionsNo << endl;
    os << g.winConditionsSome << endl;
    os << g.winConditionsNoXOnYLHS << endl;
    os << g.winConditionsNoXOnYRHS << endl;
    os << g.winConditionsSomeXOnYLHS << endl;
    os << g.winConditionsSomeXOnYRHS << endl;
    os << g.winConditionsAllXOnYLHS << endl;
    os << g.winConditionsAllXOnYRHS << endl;
    os << g.hash << endl;
    return os;
}

istream& operator>>(istream& is, Game & g) {
    is >> g.currentState; // this contains the entire level structure and is simply a deque<deque<deque<int> > >
    is >> g.beginStateAfterStationaryMove;
    is >> g.undoStates;
    is >> g.currentLevelIndex;
    is >> g.currentLevelHeight >> g.currentLevelWidth;
    is >> g.levels;
    //these are explicity left empty!
    //os << g.messages << endl;
    //os << g.currentMessageIndex << endl;
    is >> g.objPrimaryName;
    is >> g.objTexture;
    is >> g.objLayer;
    is >> g.layerCount;
    is >> g.rules;
    is >> g.synonyms;
    is >> g.aggregates;
    is >> g.properties;
    is >> g.definedNames;
    is >> g.playerIndices;
    is >> g.synsWithSingleCharName;
    is >> g.aggsWithSingleCharName;
    is >> g.winConditionsNo;
    is >> g.winConditionsSome;
    is >> g.winConditionsNoXOnYLHS;
    is >> g.winConditionsNoXOnYRHS;
    is >> g.winConditionsSomeXOnYLHS;
    is >> g.winConditionsSomeXOnYRHS;
    is >> g.winConditionsAllXOnYLHS;
    is >> g.winConditionsAllXOnYRHS;
    is >> g.hash;
    return is;
}


/*
bool operator<(const Game & lgame, const Game & rgame) {
#define gcmp(x) if(lgame.x != rgame.x) return lgame.x < rgame.x;
    gcmp(objLayer)
    gcmp(layerCount)
    gcmp(playerIndices)
    gcmp(winConditionsNo)
    gcmp(winConditionsSome)
    gcmp(winConditionsNoXOnYLHS)
    gcmp(winConditionsNoXOnYRHS)
    gcmp(winConditionsSomeXOnYLHS)
    gcmp(winConditionsSomeXOnYRHS)
    gcmp(winConditionsAllXOnYLHS)
    gcmp(winConditionsAllXOnYRHS)
    return lgame.rules < rgame.rules;
}*/


static bool parseGameLines(vector<string> lines, Game & game, Logger & logger) {
#ifndef compiledWithoutOpenframeworks
    switchToDefaultPalette();
#endif
    
    // In this order
    static vector<string> sections = {"objects","legend","sounds","collisionlayers","rules","winconditions","levels"};
    string currentSection = ""; int sectionStartsFromLine = 0;
    
    PIXEL_RESOLUTION = 5;
    int parseMode = -1;
    for(int line=0;line<lines.size();++line) {
        string str = formatString(lines[line]);
        
        if(currentSection == "") { //initial phase
            auto tokens = tokenize(str);
            if(tokens.size()>=2 && tokens[0] == "color_scheme") {
#ifndef compiledWithoutOpenframeworks
                if(!switchToPalette(tokens[1])) {
                    logger.logError("Invalid color scheme: "+tokens[1], line);
                    return false;
                }
#endif
            }
            else if(tokens.size() >= 2 && tokens[0] == "sprite_size") {
                if(isInt(tokens[1])) {
                    PIXEL_RESOLUTION = stoi(tokens[1]);
                } else {
                    logger.logError("Expected integer, but received: "+tokens[1], line);
                    return false;
                }
            } /*
               //default assumption actually
               else if(tokens.size()==1 && tokens[0] == "run_rules_on_level_start") {
               logger.logError("Currently unsupported run_rules_on_level_start");
            }*/
            else if(tokens.size()==1 && tokens[0] == "noaction") {
                logger.logError("noaction unsupported. use [action player] -> cancel instead.", line);
                return false;
            }
            else if(tokens.size()==1 && tokens[0] == "require_player_movement") {
                logger.logError("require_player_movement unsupported. Instead use: [Player] -> [Player Marker] ; late [Player Marker] -> Cancel; late [Marker] -> []", line);
                return false;
            }
            else if(tokens.size()==1 && tokens[0] == "norepeat_action") {
                logger.logWarning("norepeat_action is the default behaviour for PSMIS.", line);
                return false;
            }
            /*
            else if(tokens.size()==1 && tokens[0] == "noundo") {
                logger.logError("noundo is ignored", line);
            }*/
            //run_rules_on_level_start is on by default!!!
        }
        
        if( find(sections.begin(),sections.end(),str) != sections.end() ) {
            int sectionEndsAtLine = line-1;
            if(currentSection == "objects") {
                if(parseMode >= 0) {
                    logger.logError("Section "+sections[parseMode]+" comes before "+sections[0],line);
                    return false;
                }
                bool success = parseObjects(lines, sectionStartsFromLine, sectionEndsAtLine, game, logger);
                if(!success) return false;
                parseMode = 0;
            } else if(currentSection == "legend") {
                if(parseMode >= 1) {
                    logger.logError("Section "+sections[parseMode]+" comes before "+sections[1],line);
                    return false;
                }
                bool success = parseLegend(lines, sectionStartsFromLine, sectionEndsAtLine, game,logger);
                if(!success) return false;
                parseMode = 1;
            } else if(currentSection == "sounds") {
                if(parseMode >= 2) {
                    logger.logError("Section "+sections[parseMode]+" comes before "+sections[1],line);
                    return false;
                }
                //bool success = parseSounds(lines, sectionStartsFromLine, sectionEndsAtLine, game);
                //if(!success) return false;
                parseMode = 2;
            } else if(currentSection == "collisionlayers") {
                if(parseMode >= 3) {
                    logger.logError("Section "+sections[parseMode]+" comes before "+sections[3],line);
                    return false;
                }
                bool success = parseCollisionLayers(lines, sectionStartsFromLine, sectionEndsAtLine, game, logger);
                if(!success) return false;
                parseMode = 3;
            } else if(currentSection == "rules") {
                if(parseMode >= 4) {
                    logger.logError("Section "+sections[parseMode]+" comes before "+sections[4],line);
                    return false;
                }
                bool success = parseRules(lines, sectionStartsFromLine, sectionEndsAtLine, false, game, logger);
                if(!success) return false;
                parseMode = 4;
            } else if(currentSection == "winconditions") {
                if(parseMode >= 5) {
                    logger.logError("Section "+sections[parseMode]+" comes before "+sections[5],line);
                    return false;
                }
                bool success = parseWinconditions(lines, sectionStartsFromLine, sectionEndsAtLine, game, logger);
                if(!success) return false;
                parseMode = 5;
            }
            
            currentSection = str;
            sectionStartsFromLine = line+1;
        }
    }
    
    // has no section after it:
    if(currentSection == "levels") {
        bool success = parseLevels(lines, sectionStartsFromLine, lines.size(), game, logger);
        if(!success) return false;
        parseMode = 6;
    } else {
        logger.logError("Missing levels section.", lines.size()-1);
        return false;
    }
    
    return true;
}

static bool parseGameGeneratorLines(vector<string> generatorLines, Game & game, Logger & logger) {
    //remove "generation"
    for(int i=0;i<generatorLines.size();++i) {
        string fstr = formatString(generatorLines[i]);
        if(fstr == "generation" || fstr == "transform") generatorLines[i] = "";
    }
    
    bool success = parseRules(generatorLines, 0, generatorLines.size(), true, game, logger);
    if(!success) return false;
    
    return true;
}

//left bool = success of parsing lines, right bool = success of parsing generator
pair<bool,bool> parseGame(vector<string> lines, vector<string> generatorLines, Game & game, Logger & levelEditLogger, Logger & generateLogger) { //tries to load a level
        
    Game emptyGame;
    game = emptyGame;
    
    levelEditLogger.reset();
    generateLogger.reset();
    
    //remove comments of lines
    int commentLevel = 0;
    for(int i=0;i<lines.size();++i) {
        for(int j=0;j<lines[i].size();++j) {
            if(lines[i][j] == '(') {
                commentLevel++;
                lines[i][j] = ' ';
            }
            else if(lines[i][j] == ')' && commentLevel > 0) {
                commentLevel--;
                lines[i][j] = ' ';
            }
            else if(commentLevel > 0) lines[i][j] = ' ';
        }
    }
    
    //remove comments of generatorLines
    commentLevel = 0;
    for(int i=0;i<generatorLines.size();++i) {
        for(int j=0;j<generatorLines[i].size();++j) {
            if(generatorLines[i][j] == '(') {
                commentLevel++;
                generatorLines[i][j] = ' ';
            }
            else if(generatorLines[i][j] == ')' && commentLevel > 0) {
                commentLevel--;
                generatorLines[i][j] = ' ';
            }
            else if(commentLevel > 0) generatorLines[i][j] = ' ';
        }
    }
    
    bool successParsingLines = parseGameLines(lines, game, levelEditLogger);
    bool successParsingGeneratorLines = parseGameGeneratorLines(generatorLines, game, generateLogger);
    
    return make_pair( successParsingLines, successParsingGeneratorLines );
}



