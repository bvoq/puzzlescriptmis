#ifndef rules_h
#define rules_h

#include "macros.h"

enum Type {
    TYPE_OBJECT, TYPE_ELLIPSIS, TYPE_LHS_PROPERTY, TYPE_RHS_PROPERTY
};

enum Direction {
    DIR_NONE, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_ELLIPSIS
};

enum Commands {
    CMD_SFX0, CMD_SFX1, CMD_SFX2, CMD_SFX3, CMD_SFX4, CMD_SFX5, CMD_SFX6, CMD_SFX7, CMD_SFX8, CMD_SFX9, CMD_SFX10, CMD_CANCEL, CMD_MESSAGE, CMD_CHECKPOINT, CMD_RESTART, CMD_WIN, CMD_AGAIN
};

struct Rule {
    Direction direction; // 0b00001 = UP, 0b00010 = DOWN, 0b00100 = LEFT, 0b01000 = RIGHT, 0b10000 = ELLIPSIS
    // Encoding:

    //Ex. rule: Can only move as long as somewhere there exists a crate on a wall moving in the direction of the player:
    // [> Player | Crate ] [ Wall | Crate] -> [> Player | > Crate] [Wall | > Crate]
    vector<vector<Type> > lhsTypes, rhsTypes;
    vvvc lhsDirs, rhsDirs;
    vvvs lhsObjects, rhsObjects;
    vvvs lhsLayers, rhsLayers; // => better precomputed than looked up during each iteration
    vector<Commands> commands;
    string msgString;
    
    //an "option 0.5" rule is implicitly a "choose 1 option 0.5" rule, similarly a "choose 3" rule is implicitly a "choose 3 option 1.0" rule
    int choose = 0; //0 = not a choose rule, 1=choose 1, 2=choose 2, etc.
    float optionProb = 1.0; //1 = not an option rule

    bool late = false;
    
    int lineNumber = -1;
    int groupNumber = -1;
    int startloopNumber = -1;
    
    uint64_t getHash(uint64_t chash) const;
};

extern istream& operator>>(istream& is, Type & t);
extern istream& operator>>(istream& is, Direction & t);
extern istream& operator>>(istream& is, Commands & t);

extern ostream& operator<<(ostream& os, const Rule & r);
extern istream& operator>>(istream& is, Rule & r);
//extern vector<Rule> toRule(vector<string> strs, vector<string> keyword_names, map<string,int> keyword_to_number);
extern void printRule(ostream & o, const Rule & r, const Game & game);

extern bool operator<(const Rule & lhs, const Rule & rhs); //returns if two game rules are equivalent (not in sense of semantics, but syntax)

extern bool parseRules(vector<string> lines, int sectionStartsFromLine, int sectionEndsAtLine, bool generatorSection, Game & game, Logger & logger);


#endif /* rules_h */
