#ifndef game_h
#define game_h

#include "macros.h"

#include "rules.h"

//SoA layout, C++ data-oriented style. This code needs to be fast, no match for AoS!
//Only main thread is supposed to access elements from Game with the exception of generatorNeighborhood, which has a special mutex for that purpose.
struct Game {
    // Current state of a level (same for play mode and edit mode)
    vvvs currentState; // this contains the entire level structure and is simply a deque<deque<deque<int> > >
    vvvs beginStateAfterStationaryMove;
    vector<pair<vvvs, short> > undoStates;
    int currentLevelIndex=-1;
    int currentLevelHeight = -1, currentLevelWidth = -1;
    
    // Level
    vector<vvvs> levels;
    map<int, vector<string> > messages; //string message, int = 0 before first level, 1 = after first, before second level, etc.
    int currentMessageIndex = 0;
    
    // Objects referred to by index:
    /* RULES-PART OF THE GAME, ONLY CHANGED ONCE AT THE BEGINNING DURING PARSEGAME*/
    
    vector<string> objPrimaryName; //primary name used by the parser to give nicer error messages.
    vector<int> objTexture; //index to the colors::texture for the respective object
    vector<short> objLayer; //indicates which layer the block is.
    
    int layerCount = -1; //ofc gets overwritten by the number of collision layers.
    
    //For rules AoS is ok because each rule gets executed after the other, so they are cache-local.
    vector<Rule> rules;
    
    //Legend:
    map<string, short> synonyms; //only those with utf8 length 1 can be used when drawing a map!
    map<string, vector<short> > aggregates;
    map<string, vector<short> > properties;
    set<string> definedNames;
    vector<pair<short,short> > playerIndices;
    vector<pair<string,short> > synsWithSingleCharName;
    vector<pair<string,vector<short> > > aggsWithSingleCharName;
    
    //Win-conditions:
    //pair<short,short> := (objectIndex,objectLayer) [so no additional lookup is necessary ]
    vector<pair<short,short> > winConditionsNo;
    vector<vector<pair<short,short> > > winConditionsSome; //in every element, one has to be contained
    vector<vector<pair<short,short> > > winConditionsNoXOnYLHS;
    vector<vector<pair<short,short> > > winConditionsNoXOnYRHS;
    vector<vector<pair<short,short> > > winConditionsSomeXOnYLHS;
    vector<vector<pair<short,short> > > winConditionsSomeXOnYRHS;
    vector<vector<pair<short,short> > > winConditionsAllXOnYLHS;
    vector<vector<pair<short,short> > > winConditionsAllXOnYRHS;

    //generatorReplaceIdWith[id] = vector to replace.
    vector<Rule> generatorRules;
    
    //Defined in game.cpp
    bool insertSynonym (string str, short index, int lineno, Logger & logger);
    bool insertAggregate (string str, vector<short> indices, int lineno, Logger & logger);
    bool insertProperty (string str, vector<short> indices, int lineno, Logger & logger);
    
    void updateLevelState(vvvs newCurrentState, int index);
    
    uint64_t hash = 0; //should be private but is a struct so it doesn't do much.
    uint64_t getHash() const; //hash value of the rules
    uint64_t getTotalHash() const; //hash value of the entire game including the levels.
    inline uint64_t getGameStateHash(const vvvs & state) const;
    deque<short> getCurrentMoves() const;
    //uint64_t getTotalHash() const;
};


extern ostream& operator<<(ostream& os, const Game & g);
extern istream& operator>>(istream& os, Game & g);

//equal if two games share the same mechanics, but not necessarily the same generator.
extern bool operator<(const Game & lgame, const Game & rgame);

extern pair<bool,bool> parseGame(vector<string> lines, vector<string> generatorLines, Game & game, Logger & levelEditLogger, Logger & generatorLogger);

#endif /* game_h */
