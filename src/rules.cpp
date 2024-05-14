#include "rules.h"
#include "game.h"
#include "logError.h"
#include "objects.h"
#include "stringUtilities.h"

//!TODO: Support perpendicular & parallel rules
static set<string> keywords_modifiers = {">","<","^","v","up","down","left","right","stationary","no","randomdir","random","perpendicular","parallel","moving","horizontal","vertical","orthogonal","action"};

static set<string> keywords_modifiers_unsupported = {"randomdir","random"};

static set<string> keywords_commandwords = {"sfx0","sfx1","sfx2","sfx3","sfx4","sfx5","sfx6","sfx7","sfx8","sfx9","sfx10","cancel","checkpoint","restart","win","message","again"};

static set<string> keywords_commandwords_unsupported = {"checkpoint","win","restart"};


uint64_t Rule::getHash(uint64_t chash = INITIAL_HASH) const {
    chash = FNV64(this->direction, chash);
    for(int i=0;i<this->lhsTypes.size();++i) for(int j=0;j<this->lhsTypes[i].size();++j)
    chash = FNV64(this->lhsTypes[i][j], chash);
    
    for(int i=0;i<this->rhsTypes.size();++i) for(int j=0;j<this->rhsTypes[i].size();++j)
    chash = FNV64(this->rhsTypes[i][j], chash);

    HashVVV(this->lhsDirs, chash); HashVVV(this->rhsDirs,chash);
    HashVVV(this->lhsObjects, chash); HashVVV(this->rhsObjects,chash);
    HashVVV(this->lhsLayers, chash); HashVVV(this->rhsLayers,chash);
    
    for(int i=0;i<this->commands.size();++i)
    chash = FNV64(this->commands[i], chash);
    
	chash = FNV64(this->choose, chash);
	if (this->choose) {
		union U {
			float f;
			uint32_t q = 0x0; //has to be pre-initialised!! elsewise different hash values might pop up.
		}; //& 0xfffff000
		U u;
		u.f = this->optionProb;
		chash = FNV64(u.q, chash);
	}
    chash = FNV64(this->late, chash);
    chash = FNV64(this->groupNumber, chash);
    return chash;
}

istream& operator>>(istream& is, Type & t) {
    unsigned int i; is >> i; t = static_cast<Type>(i);
    return is;
}
istream& operator>>(istream& is, Direction & t) {
    unsigned int i; is >> i; t = static_cast<Direction>(i);
    return is;
}
istream& operator>>(istream& is, Commands & t) {
    unsigned int i; is >> i; t = static_cast<Commands>(i);
    return is;
}

ostream& operator<<(ostream& os, const Rule & r) {
    os << r.direction << endl;
    os << r.lhsTypes << endl << r.rhsTypes << endl;
    os << r.lhsDirs << endl << r.rhsDirs << endl;
    os << r.lhsLayers << endl << r.rhsLayers << endl;
    
    os << r.commands.size() << endl;
    for(size_t i=0;i<r.commands.size();++i)
        os << static_cast<unsigned int>(r.commands[i]) << endl;
    // os << r.msgString << endl; //left out cus it can contain spaces
    os << r.choose << endl;
    os << r.optionProb << endl;
    os << r.late << endl;
    os << r.lineNumber << endl;
    os << r.groupNumber << endl;
    return os;
}

istream& operator>>(istream& is, Rule & r) {
    unsigned int temp;
    is >> temp; r.direction = static_cast<Direction>(temp);
    is >> r.lhsTypes >> r.rhsTypes;
    is >> r.lhsDirs >> r.rhsDirs;
    is >> r.lhsLayers >> r.rhsLayers;
    
    size_t s; is >> s;
    for(size_t i=0;i<s;++i) {
        unsigned int ui; is >> ui;
        r.commands.push_back(static_cast<Commands>(ui));
    }
    //is >> r.msgString;
    
    is >> r.choose;
    is >> r.optionProb;
    is >> r.late;
    is >> r.lineNumber;
    is >> r.groupNumber;
    return is;
}

struct RuleUncompiled {
    vector<string> directions; //UP=0x00001,DOWN=0x00010,LEFT=0x00100,RIGHT=0x01000
    
    vector<vector<vector<string> > > lhs = {}, rhs = {};
    
    bool choose = false;
    bool option = false;
    float optionProb = 1.0; //1 = not an option rule
    int chooseCount = 0; //0 = not a choose rule, 1=choose 1, 2=choose 2, etc., if(choose) evaluates as true
    
    bool late = false;
    // unsupported
    bool rigid = false;
    bool randomRule = false;
    
    int lineNumber = -1;
    int groupNumber = -1;
    int startloopNumber = -1; // -1 = not in a startloop/endloop bracket, otherwise rule number that starts with startloop.
    
    vector<Commands> commands = {}; //potentially executed if empty
    string msgString;
};


static bool operator<(const RuleUncompiled & rl, const RuleUncompiled & rr) {
    if(rl.late != rr.late) return !rl.late;
    return rl.lineNumber < rr.lineNumber;
}

//returns empty rules if failed
static bool toUncompiledRule(vector<string> lines, int sectionStartsFromLine, int sectionEndsAtLine, vector<RuleUncompiled> & rulesUncompiled, bool generatorSection, const Game & game, Logger & logger) {

    vector<RuleUncompiled> crules;
    
    int groupNumber = 0;
    int startloopNumber = -1;

    for(int line=sectionStartsFromLine;line<sectionEndsAtLine;++line) {
        string str = formatString(lines[line]);
        if(str.size() == 0) continue; // skip empty lines
        
        //TOKENIZE
        str = replaceStrOnce(str, "->"," -> ");
        str = replaceStrOnce(str, "|"," | ");
        str = replaceStrOnce(str, "["," [ ");
        str = replaceStrOnce(str, "]"," ] ");
        str = replaceStrOnce(str, " >"," > ");
        str = replaceStrOnce(str, " <"," < ");
        vector<string> tokens = tokenize(str);
        
        if(tokens.size() == 0) {
            DEB("Spoopy empty token rule.");
            continue;
        }
        
        // single token lines
        if(tokens[0] == "startloop") {
            if(tokens.size() > 1) {
                logger.logError("startloop needs to be alone on a line..", line);
                return false;
            }
            if(startloopNumber != -1) {
                logger.logError("Cannot have nested 'startloop' commands.", line);
                return false;
            }
            startloopNumber = crules.size();
            continue;
        } else if(tokens[0] == "endloop") {
            if(tokens.size() > 1) {
                logger.logError("startloop needs to be alone on a line..", line);
                return false;
            }
            if(startloopNumber == -1) {
                logger.logError("Missing 'startloop'.", line);
                return false;
            }
            if(startloopNumber == crules.size()) {
                logger.logError("Missing content in 'startloop/endloop'.", line);
                return false;
            }
            startloopNumber = -1;
            continue;
        }
        
        //PARSE
        /*
         STATE
         0 - scanning for initial directions
         LHS
         1 - reading cell contents LHS
         2 - reading cell contents RHS
         */
        int parseState = 0;
        RuleUncompiled crule;
        crule.lineNumber = line;
        crule.groupNumber = line; //Assume it's a single standing group. updated if different
        crule.startloopNumber = startloopNumber;
        
        vector<string> ccell = {};
        vector<vector<string> > ccellrow = {};
        bool incellrow = false;
        
        for(int tokenno=0;tokenno<tokens.size();++tokenno) {
            string token = tokens[tokenno];
            if(parseState == 0) {
                if(generatorSection && (token == "late" || token == "rigid" || token == "random" )) {
                    logger.logError("The token '"+token+"' is not allowed when specifying generator constraints.", line);
                    return false;
                }
                
                if(token == "+" || token == "or") {
                    if(token == "or" && !generatorSection) {
                        logger.logError("The 'or' symbol for choosing between optional rules is only possible in the transform section.", line);
                        return false;
                    }
                    if(tokenno != 0) {
                        logger.logError("The '+' symbol, for joining a rule with the group of the previous rule, must be the first symbol on the line.", line);
                        return false;
                    } else if(crules.size() == 0) {
                        logger.logError("The '+' symbol, for joining a rule with the group of the previous rule, needs a previous rule to be applied to.",  line);
                        return false;
                    } else {
                        crule.groupNumber = crules.back().groupNumber;
                        crule.late = crules.back().late;
                        //crule.option = crules.back().option;
                        //crule.optionProb = crules.back().optionProb;
                        crule.choose = crules.back().choose;
                        crule.chooseCount = crules.back().chooseCount;
                    }
                } else if(token == "option") {
                    if(!generatorSection) {
                        logger.logError("The 'option' keyword can only be used when specifying transformation constraints.", line);
                        return false;
                    }
                    if(crule.option) {
                        logger.logError("Unexpected second 'option' token.", line);
                        return false;
                    }
                    /*
                    if(crule.groupNumber != line) {
                        logger.logError("The 'option' keyword cannot be applied to a group of statements (with +).", line);
                        return false;
                    }
                    
                    if(crule.groupNumber != line && crule.option == false) { //always false by default but touche
                        logger.logError("The 'option' keyword has to be applied to the entire group of statements.", line);
                        return false;
                    }*/
                    if(tokenno+1>=tokens.size()) {
                        logger.logError("Expected more after 'option' token.", line);
                        return false;
                    }
                    
                    string tokennext = tokens[tokenno+1];
                    
                    crule.option = true;
                    crule.optionProb = 1.0;
                    if(isFloat(tokennext)) {
                        float f = stof(tokennext);
                        if(f<0.0 || f > 1.0) {
                            logger.logError("Expected a probability between 0 and 1 instead of " + tokennext + ".", line);
                            return false;
                        }
                        crule.optionProb = f;
                        tokenno++;
                    }
                } else if(token == "choose") {
                    if(!generatorSection) {
                        logger.logError("The 'choose' keyword is only used when specifying transformation constraints.", line);
                        return false;
                    }
                    if(crule.choose) {
                        logger.logError("Unexpected second 'choose' token.", line);
                        return false;
                    }
                    /*
                    if(crule.groupNumber != line) {
                        logger.logError("The 'choose' keyword cannot be applied to a group of statements (with +).", line);
                        return false;
                    }
                    
                    if(crule.groupNumber != line && crule.choose == false) { //always false by default but touche
                        logger.logError("The 'choose' keyword has to be applied to the entire group of statements.", line);
                        return false;
                    }*/
                    
                    if(tokenno+1>=tokens.size()) {
                        logger.logError("Expected more after 'choose' token.", line);
                        return false;
                    }
                    
                    string tokennext = tokens[tokenno+1];
                    
                    if(isInt(tokennext)) {
                        int i = stoi(tokennext);
                        if(i<1) {
                            logger.logError("Cannot have a non-positive choose count '"+tokennext+"'.", line);
                            return false;
                        }
                        crule.choose = true;
                        crule.chooseCount = i;
                        tokenno++;
                    } else {
                        logger.logError("Expected an integer token after 'choose' token.", line);
                        return false;
                    }
                } else if(token == "left" || token == "right" || token == "up" || token == "down") {
                    crule.directions.push_back(token);
                } else if(token == "horizontal") {
                    crule.directions.push_back("left");crule.directions.push_back("right");
                } else if(token == "vertical") {
                    crule.directions.push_back("up");crule.directions.push_back("down");
                } else if(token == "orthogonal") {
                    crule.directions.push_back("up");crule.directions.push_back("down");
                    crule.directions.push_back("left");crule.directions.push_back("right");
                } else if(token == "late") {
                    crule.late = true;
                } else if(token == "rigid") {
                    crule.rigid = true;
                } else if(token == "random") {
                    logger.logError("Random rules are currently not supported.", line);
                    return false;
                    
                    //crule.randomRule = true;
                } else if(token == ">"||token=="<"||token=="v"||token=="^") {
                    logger.logError("You cannot use relative directions (\"^v<>\") to indicate in which direction(s) a rule applies.  Use absolute directions indicator (UP,LEFT,RIGHT,DOWN) instead.", line);
                    return false;
                } else if (token == "[") {
                    if (crule.directions.size() == 0) {
                        crule.directions.push_back("up");crule.directions.push_back("down");
                        crule.directions.push_back("left");crule.directions.push_back("right");
                    }
                    parseState = 1;
                    tokenno--;
                } else {
                    logger.logError("Unexpected token: '"+token+"'.", line);
                    return false;
                }
            }
            else if(parseState == 1 || parseState == 2) {
                if(token == "[") {
                    if (ccell.size() > 0) {
                        logger.logError("Error, malformed cell rule - encountered a '[' before previous bracket was closed", line);
                        return false;
                    }
                    incellrow = true;
                    ccell.clear();
                } else if(keywords_modifiers.count(token) != 0) {
                    if(keywords_modifiers_unsupported.count(token) != 0) {
                        logger.logError("'"+token+"' is not supported for this engine (I prefer it to be deterministic).", line);
                        return false;
                    }
                    if (ccell.size() % 2 == 1) {
                        logger.logError("Error, an item can only have one direction/action at a time, but you're looking for several at once!", line);
                        return false;
                    } else if (!incellrow) {
                        logger.logError("Invalid syntax. Directions should be placed at the start of a rule.", line);
                        return false;
                    } else if(crule.late && !(token == "no" || token == "random")) {
                        logger.logError("Movements cannot appear in late rules.", line);
                        return false;
                    } else {
                        ccell.push_back(token);
                    }
                } else if (token == "|") {
                    if (ccell.size() % 2 == 1) {
                        logger.logError("In a rule, if you specify a force, it has to act on an object.", line);
                        return false;
                    } else {
                        ccellrow.push_back(ccell);
                        ccell.clear();
                    }
                } else if(token == "]") {
                    if (ccell.size() % 2 == 1) {
                        if (ccell[0]=="...") {
                            logger.logError("Cannot end a rule with ellipses.", line);
                            return false;
                        } else {
                            logger.logError("In a rule, if you specify a force, it has to act on an object.", line);
                            return false;
                        }
                    } else {
                        ccellrow.push_back(ccell);
                        ccell.clear();
                    }
                    
                    if (parseState == 1) //lhs
                        crule.lhs.push_back(ccellrow);
                    else if(parseState == 2) //rhs
                        crule.rhs.push_back(ccellrow);
                    
                    ccellrow.clear();
                    incellrow = false;
                } else if(token == "->") {
                    if (parseState != 1) {
                        logger.logError("Error, you can only use '->' once in a rule; it\'s used to separate before and after states.", line);
                        return false;
                    } if (ccellrow.size() > 0) {
                        logger.logError("Encountered an unexpected '->' inside square brackets.  It\'s used to separate states, it has no place inside them >:| .", line);
                        return false;
                    } else {
                        parseState = 2;
                    }
                }  else if( game.synonyms.count(token) != 0 || game.aggregates.count(token) != 0 || game.properties.count(token) != 0) {
                    if(token == "background") {
                        logger.logWarning("Background should not be used in rules.", line);
                    }
                    if(!incellrow) {
                        logger.logError("Invalid token: '"+token+"'. Object names should only be used within cells (square brackets).", line);
                        return false;
                    } else if(ccell.size() % 2 == 0) {
                        ccell.push_back("");
                        ccell.push_back(token);
                    } else if(ccell.size() % 2 == 1){
                        ccell.push_back(token);
                    }
                } else if(token == "...") {
                    if (!incellrow) {
                        logger.logError("Invalid syntax, ellipses should only be used within cells (square brackets).", line);
                        return false;
                    } else {
                        ccell.push_back(token);
                        ccell.push_back(token);
                    }
                } else if (keywords_commandwords.count(token) != 0) {
                    if (parseState != 2) {
                        logger.logError("Commands cannot appear on the left-hand side of the arrow.", line);
                        return false;
                    }
                    if(keywords_commandwords_unsupported.count(token) != 0) {
                        logger.logError("The keyword "+token+" is not supported.", line);
                        return false;
                    }
                    
                    if(crule.option || crule.choose) {
                        logger.logError("Cannot combine an 'option' or 'choose' rule together with commands.", line);
                        return false;
                    }
                    
                    if (token=="message") {
                        //we have to find the original message before tokenizing.
                        size_t index = lines[line].find(token);
                        if (index!=string::npos)
                            crule.msgString = lines[line].substr(index);
                        else
                            crule.msgString = "";
                        
                        crule.commands.push_back(CMD_MESSAGE); //handled slightly differently than in puzzlescript engine
                        tokenno = (int)tokens.size();
                    }
                    else if(token=="sfx0") crule.commands.push_back(CMD_SFX0);
                    else if(token=="sfx1") crule.commands.push_back(CMD_SFX1);
                    else if(token=="sfx2") crule.commands.push_back(CMD_SFX2);
                    else if(token=="sfx3") crule.commands.push_back(CMD_SFX3);
                    else if(token=="sfx4") crule.commands.push_back(CMD_SFX4);
                    else if(token=="sfx5") crule.commands.push_back(CMD_SFX5);
                    else if(token=="sfx6") crule.commands.push_back(CMD_SFX6);
                    else if(token=="sfx7") crule.commands.push_back(CMD_SFX7);
                    else if(token=="sfx8") crule.commands.push_back(CMD_SFX8);
                    else if(token=="sfx9") crule.commands.push_back(CMD_SFX9);
                    else if(token=="sfx10") crule.commands.push_back(CMD_SFX10);
                    else if(token=="cancel") crule.commands.push_back(CMD_CANCEL);
                    
                    else if(token=="checkpoint") crule.commands.push_back(CMD_CHECKPOINT);
                    else if(token=="restart") crule.commands.push_back(CMD_RESTART);
                    else if(token=="win") crule.commands.push_back(CMD_WIN);
                    else if(token=="again") crule.commands.push_back(CMD_AGAIN);
                }
                else {
                    logger.logError("Unexpected token: '"+token+"'.", line);
                    return false;
                }
            }
        }
        
        if(crule.rhs.size() != 0) {
            if(crule.lhs.size() != crule.rhs.size()) {
                logger.logError("There must be an equal number of groups [] on the left and right-hand side of the rule.", line);
                return false;
            }
            
            for(int i=0;i<crule.lhs.size();++i) {
                if(crule.lhs[i].size() != crule.rhs[i].size()) {
                    logger.logError("There must be an equal number of cells within a [] on the left and right-hand side of the rule.", line);
                    return false;
                }
            }
        }
        
        //DEBUGGING PURPOSES
        cout << "[" << crule.lineNumber << "] " << lines[crule.lineNumber] << endl;
        
        crules.push_back(crule);
    }
    
    if(startloopNumber != -1) {
        logger.logError("Missing endloop.", sectionEndsAtLine - 1);
        return false;
    }
    
    
    //first expand all rules that are of the form no property => no prop1 no prop2, etc.
    for(auto & r: crules) {
        
        for(auto & sss : r.lhs) {
            for(auto & ss : sss) {
                vector<string> newV;
                for(int l=0;l<ss.size();l+=2) {
                    if(ss[l] == "no" && game.properties.count(ss[l+1]) != 0) {
                        for(short objIndex : game.properties.at( ss[l+1] )) {
                            newV.push_back( "no" );
                            newV.push_back( game.objPrimaryName [objIndex] );
                        }
                    } else {
                        newV.push_back(ss[l]  );
                        newV.push_back(ss[l+1]);
                    }
                }
                ss = newV;
            }
        }
        for(auto & sss : r.rhs) { //code which has been copied twice, careful about updating it
            for(auto & ss : sss) {
                vector<string> newV;
                for(int l=0;l<ss.size();l+=2) {
                    if(ss[l] == "no" && game.properties.count(ss[l+1]) != 0) {
                        for(short objIndex : game.properties.at( ss[l+1] )) {
                            newV.push_back( "no" );
                            newV.push_back( game.objPrimaryName [objIndex] );
                        }
                    } else {
                        newV.push_back(ss[l]  );
                        newV.push_back(ss[l+1]);
                    }
                }
                ss = newV;
            }
        }
    }
    
    
    vector<RuleUncompiled> newrules;
    //expand all properties into a bunch of rules
    for(auto r : crules) {
        // create a rule that is full except for the properties which are left empty:
        RuleUncompiled noPropertyRule = r;
        vector<RuleUncompiled> expandedrules = {r};
        int numberOfRules = 1;
        
        for(int i=0;i<r.lhs.size();++i) {
            for(int j=0;j<r.lhs[i].size();++j) {
                for(int k=0;k<r.lhs[i][j].size();++k) {
                    if(game.properties.count(r.lhs[i][j][k]) != 0) {
                        vector<RuleUncompiled> newrules;
                        for(short objIndex : game.properties.at( r.lhs[i][j][k] )) {
                            for(RuleUncompiled ru : expandedrules) {
                                ru.lhs[i][j][k] = game.objPrimaryName[objIndex];
                                newrules.push_back( ru );
                            }
                        }
                        expandedrules = newrules;
                    }
                }
            }
        }

        //Updating the RHS according to the LHS properties that have been found.
        for(int i=0;i<r.rhs.size();++i) {
            for(int j=0;j<r.rhs[i].size();++j) {
                for(int k=1;k<r.rhs[i][j].size();k+=2) {
                    if(game.properties.count(r.rhs[i][j][k]) != 0) {
                        //first check at r to see whether the property is directly
                        int foundProperty = 0;
                        for(int lk=1;lk<r.lhs[i][j].size(); lk+=2) {
                            if(r.lhs[i][j][lk] == r.rhs[i][j][k]) {
                                for(RuleUncompiled & ru : expandedrules) {
                                    ru.rhs[i][j][k] = ru.lhs[i][j][lk];
                                }
                                foundProperty++;
                            }
                        }
                        
                        if(foundProperty == 0) {
                            for(int li=0;li<r.lhs.size();++li) {
                                for(int lj=0;lj<r.lhs[li].size();++lj) {
                                    for(int lk=1;lk<r.lhs[li][lj].size();++lk) {
                                        if(r.lhs[li][lj][lk] == r.rhs[i][j][k]) {
                                            for(RuleUncompiled & ru : expandedrules) {
                                                ru.rhs[i][j][k] = ru.lhs[li][lj][lk];
                                            }
                                            foundProperty++;
                                        }
                                    }
                                }
                            }
                        }
                        
                        if(foundProperty != 1) {
                            logger.logError("Property '"+r.rhs[i][j][k]+"' on RHS can't be inferred from the LHS.", r.lineNumber);
                            return false;
                        }
                    }
                }
            }
        }
        
        copy (expandedrules.begin(),expandedrules.end(),back_inserter(newrules));
    }

    crules = newrules;

    
    //check that all rules with + are either all late or not.
    for(int i=1;i<crules.size();++i) {
        if(crules[i].groupNumber != crules[i].lineNumber) {
            if(crules[i-1].late != crules[i].late) {
                logger.logError("Cannot both have a group of rules consisting out of late and non-late rules. Make sure to make the first rule late.", crules[i].lineNumber);
                return false;
            }
        }
    }
    
    //check that all rules in startloop/endloop are either all late or not.
    for(int i=1;i<crules.size();++i) {
        if(crules[i-1].startloopNumber != -1 && crules[i-1].startloopNumber == crules[i].startloopNumber) {
            if(crules[i-1].late != crules[i].late) {
                logger.logError("Cannot mix late and non-late rules in a startloop/endloop block.", crules[i].lineNumber);
                return false;
            }
        }
    }
    
    sort(crules.begin(),crules.end()); //shift all late rules to the end
    
    
    rulesUncompiled = crules;
    //renumber groupNumber to fit late description
    /*
    map<int, int> renumber;
    for(int i=0;i<crules.size();++i) {
        //cout << "[" << crules[i].groupNumber << "|" << to_string(crules[i].lineNumber) << "] " <<lines[crules[i].lineNumber] << endl;
        if(crules[i].groupNumber == crules[i].lineNumber) {
            int newIndex = crules[i].late ? lateRulesUncompiled.size() : normalRulesUncompiled.size();
            renumber[crules[i].groupNumber] = newIndex;
            crules[i].groupNumber = newIndex;
        } else {
            assert(renumber.count(crules[i].groupNumber) != 0);
            crules[i].groupNumber = renumber[crules[i].groupNumber];
        }
        
        if(crules[i].late)
            lateRulesUncompiled.push_back(crules[i]);
        else
            normalRulesUncompiled.push_back(crules[i]);
    }*/

    return true;
}


/*    Direction direction;
 // _ADIR Lay1  Lay2  Lay3  Lay4  Lay5  Lay6  Lay7
 // 00000 00000 00000 00000 00000 00000 00000 00000
 
 vector<vector<short> > lhs, rhs; //Ex. rule: Can only move as long as somewhere there exists a crate on a wall moving in the direction of the player: [> Player | Crate] [ Wall | Crate] -> [> Player | > Crate] [Wall | > Crate]
 
 bool late = false;
 
 int lineNumber = -1;
 int groupNumber = -1;
 vector<string> commands = {}; //potentially executed if not empty (most often than not it's )
 */

static bool toCompiledRule(vector<RuleUncompiled> rulesUncompiled, vector<Rule> & rules, const Game & game, Logger & logger) {
    
    //First expand the various rules.
    for(int i = 0; i < rulesUncompiled.size(); ++i) {
        
        // do them in order:
        sort(rulesUncompiled[i].directions.begin(), rulesUncompiled[i].directions.end());
        
        for(string dir : rulesUncompiled[i].directions) {
            Rule crule;
            //an "option 0.5" rule is implicitly a "choose 1 option 0.5" rule, similarly a "choose 3" rule is implicitly a "choose 3 option 1.0" rule if nothing is specified.
            crule.choose = 0;
            if(rulesUncompiled[i].option) crule.choose = 1;
            crule.optionProb = rulesUncompiled[i].optionProb;
            if(rulesUncompiled[i].choose) crule.choose = rulesUncompiled[i].chooseCount;

            crule.late = rulesUncompiled[i].late;
            crule.lineNumber = rulesUncompiled[i].lineNumber;
            crule.groupNumber = rulesUncompiled[i].groupNumber;
            crule.startloopNumber = rulesUncompiled[i].startloopNumber;
            crule.commands = rulesUncompiled[i].commands;
            crule.msgString = rulesUncompiled[i].msgString;
            
            
            //crule.commands = inrules[i].commands;
            
            static map<pair<string,string>, string> relativeMap =
            {
                // relative directions
                {{"right","^"},"up"},{{"right","v"},"down"},{{"right","<"},"left"},{{"right",">"},"right"},
                {{"up","^"},"left"},{{"up","v"},"right"},{{"up","<"},"down"},{{"up",">"},"up"},
                {{"down","^"},"right"},{{"down","v"},"left"},{{"down","<"},"up"},{{"down",">"},"down"},
                {{"left","^"},"down" },{{"left","v"},"up"},{{"left","<"},"right"},{{"left",">"},"left"},
                
                //absolute directions
                {{"up","up"},"up"},  {{"up","down"},"down"},{{"up","left"},"left"}, {{"up","right"},"right"},
                {{"down","up"},"up"},{{"down","down"},"down"},{{"down","left"},"left"},{{"down","right"},"right"},
                {{"left","up"},"up"},{{"left","down"},"down"},{{"left","left"},"left"},{{"left","right"},"right"},
                {{"right","up"},"up"},{{"right","down"},"down"},{{"right","left"},"left"},{{"right","right"},"right"},
                
                //empty direction (which means any)
                {{"up",""},""},{{"down",""},""},{{"left",""},""},{{"right",""},""},
                
                //ellipsis
                {{"up","..."},"..."},{{"down","..."},"..."},{{"left","..."},"..."},{{"right","..."},"..."},
                
                {{"up","no"},"no"},{{"down","no"},"no"},{{"left","no"},"no"},{{"right","no"},"no"},

                {{"up","action"},"action"},{{"down","action"},"action"},{{"left","action"},"action"},{{"right","action"},"action"},
                
            {{"up","stationary"},"stationary"},{{"down","stationary"},"stationary"},{{"left","stationary"},"stationary"},{{"right","stationary"},"stationary"},
                
            {{"up","horizontal"},"horizontal"},{{"down","horizontal"},"horizontal"},{{"left","horizontal"},"horizontal"},{{"right","horizontal"},"horizontal"},
                
            {{"up","vertical"},"vertical"},{{"down","vertical"},"vertical"},{{"left","vertical"},"vertical"},{{"right","vertical"},"vertical"},
                
            {{"up","parallel"},"vertical"},{{"down","parallel"},"vertical"},{{"left","parallel"},"horizontal"},{{"right","parallel"},"horizontal"},
                
            {{"up","perpendicular"},"horizontal"},{{"down","perpendicular"},"horizontal"},{{"left","perpendicular"},"vertical"},{{"right","perpendicular"},"vertical"},
                
            {{"up","moving"},"orthogonal"},{{"down","moving"},"orthogonal"},{{"left","moving"},"orthogonal"},{{"right","moving"},"orthogonal"},

            {{"up","orthogonal"},"orthogonal"},{{"down","orthogonal"},"orthogonal"},{{"left","orthogonal"},"orthogonal"},{{"right","orthogonal"},"orthogonal"}
            };
            
            if(dir == "up") {
                crule.direction = DIR_UP;
            } else if(dir == "down") {
                crule.direction = DIR_DOWN;
            } else if(dir == "left") {
                crule.direction = DIR_LEFT;
            } else if(dir == "right") {
                crule.direction = DIR_RIGHT;
            }
            
            // LHS
            for(int j=0; j<rulesUncompiled[i].lhs.size(); ++j) { // between []
                vector<Type> outrulescellType;
                vector<vector<short > > outrulescellDir;
                vector<vector<short> > outrulescellObject;
                vector<vector<short> > outrulescellLayer;
                
                for (int k=0; k<rulesUncompiled[i].lhs[j].size(); ++k) { //between |
                    assert(rulesUncompiled[i].lhs[j][k].size() % 2 == 0);
                    //assume empty cell by default [> Player | ] -> [Player | Player] example
                    outrulescellType.push_back(TYPE_OBJECT); //unless changed otherwise
                    outrulescellDir.push_back({});
                    outrulescellObject.push_back({});
                    outrulescellLayer.push_back({});
                    for(int l=0; l<rulesUncompiled[i].lhs[j][k].size(); l+=2) { //between multiple objects inside |, for e.g. ..| > Crate < Player |..
                    
                        string adir = relativeMap[  {dir, rulesUncompiled[i].lhs[j][k][l]}  ];
                        
                        short tokenDir = 0; //stationary, no movement!
                        
                        if(adir == "") tokenDir = NO_OR_ORTHOGONAL_MOVE;
                        else if(adir == "stationary") tokenDir = STATIONARY_MOVE;
                        else if(adir == "up") tokenDir = UP_MOVE;
                        else if(adir == "down") tokenDir = DOWN_MOVE;
                        else if(adir == "left") tokenDir = LEFT_MOVE;
                        else if(adir == "right") tokenDir = RIGHT_MOVE;
                        else if(adir == "vertical") tokenDir = VERTICAL_MOVE;
                        else if(adir == "horizontal") tokenDir = HORIZONTAL_MOVE;
                        else if(adir == "orthogonal") tokenDir = ORTHOGONAL_MOVE;
                        else if(adir == "action") tokenDir = ACTION_MOVE;
                        else if(adir == "no") tokenDir = RE_MOVE;
                        else if(adir != "...") {
                            logger.logError("Currently unsupported direction '"+adir+"'.", rulesUncompiled[i].lineNumber);
                            return false;
                        }
                        if(adir == "...") {
                            outrulescellType.back() = TYPE_ELLIPSIS;
                            
                            if(rulesUncompiled[i].lhs[j][k][1] != "...") {
                                logger.logError("An ellipsis on the left must be matched by one in the corresponding place on the right.", rulesUncompiled[i].lineNumber);
                                return false;
                            }
                            
                            if(rulesUncompiled[i].rhs[j][k].size() > 2) {
                                logger.logError("You can't have anything in a cell together with an ellipsis '...'.", rulesUncompiled[i].lineNumber);
                                return false;
                            }
                        } else { //DIR_NONE,DIR_UP,DIR_DOWN,DIR_LEFT,DIR_RIGHT
                            string keyword = rulesUncompiled[i].lhs[j][k][l+1];
                            if(game.properties.count(keyword) != 0) {
                                DEB("There shouldn't be any properties left, but still found: " +keyword + " in rule on line " + to_string(rulesUncompiled[i].lineNumber) );
                                exit(1);
                                
                                /*
                                if(inrules[i].lhs[j][k].size() != 2) {
                                    logger.logError("This engine does not support properties and other keywords within the same cell. Sorry, I feel it is more elegant this way. Hit me an email if this is an issue for you.", inrules[i].lineNumber);
                                    return false;
                                }
                                outrulescellType.back() = TYPE_LHS_PROPERTY;
                                for(int index : game.properties[ keyword ]) {
                                    outrulescellDir.back().push_back( tokenDir );
                                    outrulescellObject.back().push_back( index );
                                }
                                */
                            } else if(game.synonyms.count(keyword) != 0) {
                                outrulescellDir.back().push_back(tokenDir);
                                outrulescellObject.back().push_back(game.synonyms.at(keyword) );
                                outrulescellLayer.back().push_back(game.objLayer[game.synonyms.at(keyword)]);
                            } else if(game.aggregates.count(keyword) != 0) {
                                for(int index : game.aggregates.at(keyword)) {
                                    outrulescellDir.back().push_back( tokenDir );
                                    outrulescellObject.back().push_back( index );
                                    outrulescellLayer.back().push_back( game.objLayer[index] );
                                }
                            } else {
                                DEB("This shouldn't happen... keyword: " +keyword);
                                exit(1);
                            }
                        }
                    }
                }
                crule.lhsTypes.push_back(outrulescellType);
                crule.lhsDirs.push_back(outrulescellDir);
                crule.lhsObjects.push_back(outrulescellObject);
                crule.lhsLayers.push_back(outrulescellLayer);
            }
            
            
            
            // RHS
            for(int j=0; j<rulesUncompiled[i].rhs.size(); ++j) { // between []
                vector<Type> outrulescellType;
                vector<vector<short> > outrulescellDir;
                vector<vector<short> > outrulescellObject;
                vector<vector<short> > outrulescellLayer;

                for (int k=0; k<rulesUncompiled[i].rhs[j].size(); ++k) { //between |
                    assert(rulesUncompiled[i].rhs[j][k].size() % 2 == 0);
                    //assume empty cell by default [> Player | ] -> [Player | Player] example
                    outrulescellType.push_back(TYPE_OBJECT); //unless changed otherwise
                    outrulescellDir.push_back({});
                    outrulescellObject.push_back({});
                    outrulescellLayer.push_back({});

                    for(int l=0; l<rulesUncompiled[i].rhs[j][k].size(); l+=2) { //between multiple objects inside |, for e.g. ..| > Crate < Player |..
                        
                        string adir = relativeMap[   {dir, rulesUncompiled[i].rhs[j][k][l]}  ];
                        
                        short tokenDir = 0; //stationary, no movement!
                        
                        if(adir == "") tokenDir = NO_OR_ORTHOGONAL_MOVE;
                        else if(adir == "stationary") tokenDir = STATIONARY_MOVE;
                        else if(adir == "up") tokenDir = UP_MOVE;
                        else if(adir == "down") tokenDir = DOWN_MOVE;
                        else if(adir == "left") tokenDir = LEFT_MOVE;
                        else if(adir == "right") tokenDir = RIGHT_MOVE;
                        else if(adir == "action") tokenDir = ACTION_MOVE;
                        else if(adir == "no") tokenDir = RE_MOVE;
                        else if(adir != "...") {
                            logger.logError("RHS does not allow direction moving, vertical or orthogonal. Replace it with a group.", rulesUncompiled[i].lineNumber);
                            return false;
                        }
                        if(adir == "...") {
                            outrulescellType.back() = TYPE_ELLIPSIS;
                            if(rulesUncompiled[i].rhs[j][k].size() > 2) {
                                logger.logError("You can't have anything in a cell together with an ellipsis '...'.", rulesUncompiled[i].lineNumber);
                                return false;
                            }
                        } else { //DIR_NONE,DIR_UP,DIR_DOWN,DIR_LEFT,DIR_RIGHT
                            string keyword = rulesUncompiled[i].rhs[j][k][l+1];
                            if(game.properties.count(keyword) != 0) {
                                DEB("There shouldn't be any properties left, but still found: " +keyword + " in rule on line " + to_string(rulesUncompiled[i].lineNumber) );
                                exit(1);
                                /*
                                if(inrules[i].rhs[j][k].size() != 2) {
                                    logger.logError("This engine does not support properties and aggregates/synonyms within the same cell. Sorry, I feel it is more elegant this way. Hit me an email if this is an issue for you though.", inrules[i].lineNumber);
                                    return false;
                                }
                                outrulescellType.back() = TYPE_RHS_PROPERTY;
                                // TODO! Now instead of being the index to the blocks of the property it is simply an index to whichever LHS_PROPERTY matched.
                                short lhsProperties=0, lhsPropertyLocation=0; //count number of properties in group LHS
                                for(int lhsk = 0; lhsk < crule.lhsTypes.size(); ++lhsk) {
                                    if(crule.lhsTypes[j][lhsk] == TYPE_LHS_PROPERTY) {
                                        lhsProperties++;
                                        lhsPropertyLocation=lhsk;
                                    }
                                }
                                
                                if(lhsProperties == 0) {
                                    logger.logError("This rule has a property on the right-hand side, '"+keyword+"' that can't be inferred from the left-hand side. (either for every property on the right there has to be a corresponding one on the left in the same cell, OR, if there's a single occurrence of a particular property name on the left, all properties of the same name on the right are assumed to be the same).", inrules[i].lineNumber);
                                    return false;
                                }
                                
                                if(lhsProperties == 1) {
                                    outrulescellDir.back().push_back( {tokenDir} );
                                    outrulescellObject.back().push_back( {lhsPropertyLocation} );
                                } else {
                                    if(crule.lhsTypes[j][k] != TYPE_LHS_PROPERTY) {
                                        logger.logError("This rule has a property on the right-hand side, '"+keyword+"' that can't be inferred from the left-hand side. (either for every property on the right there has to be a corresponding one on the left in the same cell, OR, if there's a single occurrence of a particular property name on the left, all properties of the same name on the right are assumed to be the same).", inrules[i].lineNumber);
                                        return false;
                                    }
                                    outrulescellDir.back().push_back( {tokenDir} );
                                    outrulescellObject.back().push_back( {(short)k} );
                                }
                                 */
                                
                            } else if(game.synonyms.count(keyword) != 0) {
                                outrulescellDir.back().push_back(tokenDir);
                                outrulescellObject.back().push_back(game.synonyms.at( keyword ) );
                                outrulescellLayer.back().push_back(game.objLayer[game.synonyms.at( keyword )]);

                            } else if(game.aggregates.count(keyword) != 0) {
                                for(int index : game.aggregates.at( keyword )) {
                                    outrulescellDir.back().push_back( tokenDir );
                                    outrulescellObject.back().push_back( index );
                                    outrulescellLayer.back().push_back( game.objLayer[index] );
                                }
                                
                            } else {
                                DEB("This shouldn't happen...");
                                exit(1);
                            }
                        }
                    }
                }
                crule.rhsTypes.push_back(outrulescellType);
                crule.rhsDirs.push_back(outrulescellDir);
                crule.rhsObjects.push_back(outrulescellObject);
                crule.rhsLayers.push_back(outrulescellLayer);

            }
            /*
            if(crule.late) {
                cout << "contains late rule!!" << endl;
                printRule(cout, crule, game);
            }*/
            
            rules.push_back(crule);
        }
    }
    
    
    // Convert all left rules to right rules by simply mirroring both rules
    for(int i=0;i<rules.size();++i) {
        if(rules[i].direction == DIR_LEFT || rules[i].direction == DIR_UP) {
            for(int s=0;s<rules[i].lhsTypes.size(); ++s) {
            reverse(rules[i].lhsTypes[s].begin(), rules[i].lhsTypes[s].end());
            reverse(rules[i].lhsDirs[s].begin(), rules[i].lhsDirs[s].end());
            reverse(rules[i].lhsObjects[s].begin(), rules[i].lhsObjects[s].end());
            reverse(rules[i].lhsLayers[s].begin(), rules[i].lhsLayers[s].end());
            }
            for(int s=0;s<rules[i].rhsTypes.size(); ++s) {
                reverse(rules[i].rhsTypes[s].begin(), rules[i].rhsTypes[s].end());
                reverse(rules[i].rhsDirs[s].begin(), rules[i].rhsDirs[s].end());
                reverse(rules[i].rhsObjects[s].begin(), rules[i].rhsObjects[s].end());
                reverse(rules[i].rhsLayers[s].begin(), rules[i].rhsLayers[s].end());
            }
            
            if(rules[i].direction == DIR_LEFT) rules[i].direction = DIR_RIGHT;
            if(rules[i].direction == DIR_UP) rules[i].direction = DIR_DOWN;
        }
    }
    
    //sanity checks
    for(int i=0; i<rules.size();++i) if(rules[i].lhsDirs.size() == 0) {
        logger.logError("Left hand-side of rule needs to have at least one cell.", rules[i].lineNumber);
        return false;
    }
    
    set<string> rulesAsStrings;
    vector<Rule> oldrules = rules;
    rules.clear();
    stringstream ss;
    for(int i=0;i<oldrules.size();++i) {
        printRule(ss, oldrules[i], game);
        string ruleasstring;
        getline(ss,ruleasstring);
        if(rulesAsStrings.count(ruleasstring) == 0) {
            rulesAsStrings.insert(ruleasstring);
            rules.push_back(oldrules[i]);
        }
    }
    
    
    //renumber group rules
    map<int,int> groupNumbers;
    for(int ri=0;ri<rules.size();++ri) {
        if(groupNumbers.count(rules[ri].groupNumber) == 0) {
            int newGroupNumber = ri;
            groupNumbers[rules[ri].groupNumber] = newGroupNumber;
        }
        rules[ri].groupNumber = groupNumbers[rules[ri].groupNumber];
    }
    
    //renumber startloop rules
    map<int,int> startloopNumbers;
    for(int ri=0;ri<rules.size();++ri) {
        if(rules[ri].startloopNumber != -1) {
            if(startloopNumbers.count(rules[ri].startloopNumber) == 0) {
                int newStartLoopNumber = ri;
                startloopNumbers[rules[ri].startloopNumber] = newStartLoopNumber;
            }
            rules[ri].startloopNumber = startloopNumbers[rules[ri].startloopNumber];
        }
    }
    
    // DEBUGGING PURPOSES
    /*
    cout << "==============" << endl;
    cout << "EXPANDED RULES" << endl;
    cout << "==============" << endl;
    
    for(const Rule & r : rules) {
        printRule(cout, r, game);
    }*/
    
    
    return true;
}


void printRule(ostream & o, const Rule & r, const Game & game) {
    o << "<" << r.groupNumber << "> ";
    if(r.late) o << "late ";
    
    if(r.direction == DIR_UP) o << "UP ";
    if(r.direction == DIR_DOWN) o << "DOWN ";
    if(r.direction == DIR_LEFT) o << "LEFT ";
    if(r.direction == DIR_RIGHT) o << "RIGHT ";
    
    if(r.choose) o << "choose " << r.choose << " option " << r.optionProb << " ";

    for(int s=0;s<r.lhsObjects.size();++s) {
        o << "[";
        for(int t=0;t<r.lhsObjects[s].size();++t) {
            if(r.lhsTypes[s][t] == TYPE_ELLIPSIS) o << "...";
            else for(int u=0;u<r.lhsObjects[s][t].size();++u) {
                     if(r.lhsDirs[s][t][u] == STATIONARY_MOVE)               o <<" stationary ";
                else if(r.lhsDirs[s][t][u] == UP_MOVE)               o << " up ";
                else if(r.lhsDirs[s][t][u] == DOWN_MOVE)             o << " down ";
                else if(r.lhsDirs[s][t][u] == LEFT_MOVE)             o << " left ";
                else if(r.lhsDirs[s][t][u] == RIGHT_MOVE)            o << " right ";
                else if(r.lhsDirs[s][t][u] == VERTICAL_MOVE)         o << " vertical ";
                else if(r.lhsDirs[s][t][u] == HORIZONTAL_MOVE)       o << " horizontal ";
                else if(r.lhsDirs[s][t][u] == ORTHOGONAL_MOVE)       o << " orthogonal ";
                else if(r.lhsDirs[s][t][u] == ACTION_MOVE)           o << " action ";
                else if(r.lhsDirs[s][t][u] == RE_MOVE)               o << " no ";
                else if(r.lhsDirs[s][t][u] == NO_OR_ORTHOGONAL_MOVE) o << " ";
                else assert(false);
                    
                o << game.objPrimaryName[ r.lhsObjects[s][t][u] ];
            }
            if(t+1!=r.lhsObjects[s].size()) o << "|";
        }
        o << "]";
    }
    o << " -> ";
    for(int s=0;s<r.rhsObjects.size();++s) {
        o << "[";
        for(int t=0;t<r.rhsObjects[s].size();++t) {
            if(r.rhsTypes[s][t] == TYPE_ELLIPSIS) o << "...";
            else for(int u=0;u<r.rhsObjects[s][t].size();++u) {
                if(r.rhsDirs[s][t][u] == STATIONARY_MOVE) o <<" stationary ";
                else if(r.rhsDirs[s][t][u] == UP_MOVE) o << " up ";
                else if(r.rhsDirs[s][t][u] == DOWN_MOVE) o << " down ";
                else if(r.rhsDirs[s][t][u] == LEFT_MOVE) o << " left ";
                else if(r.rhsDirs[s][t][u] == RIGHT_MOVE) o << " right ";
                else if(r.rhsDirs[s][t][u] == ORTHOGONAL_MOVE) o << " orthogonal ";
                else if(r.rhsDirs[s][t][u] == ACTION_MOVE) o << " action ";
                else if(r.rhsDirs[s][t][u] == RE_MOVE) o << " no ";
                else if(r.rhsDirs[s][t][u] == NO_OR_ORTHOGONAL_MOVE) o << " ";
                else assert(false);
                o << game.objPrimaryName[ r.rhsObjects[s][t][u] ];
            }
            if(t+1!=r.rhsObjects[s].size()) o << "|";
        }
        o << "]";
    }
    for(Commands cmd : r.commands) {
        if(cmd == CMD_CANCEL) o << " CANCEL";
        else if(cmd == CMD_AGAIN) o << " AGAIN";
        else if(cmd == CMD_WIN) o << " WIN";
        else o << " OTHER_CMD";
    }
    o << endl;
}

bool operator<(const Rule & lhs, const Rule & rhs) { //returns if two game rules are the same (except for lineNumber)
#define rcmp(x) if(lhs.x != rhs.x) return lhs.x < rhs.x;
    rcmp(direction)
    rcmp(lhsTypes) rcmp(rhsTypes)
    rcmp(lhsObjects) rcmp(rhsObjects)
    rcmp(lhsLayers) rcmp(rhsLayers)
    rcmp(commands)
    rcmp(msgString)
    rcmp(choose)
    rcmp(optionProb)
    rcmp(late)
    rcmp(groupNumber)
    return false;
}

bool parseRules(vector<string> lines, int sectionStartsFromLine, int sectionEndsAtLine, bool generatorSection, Game & game, Logger & logger) {
    //check if completeley empty
    bool completelyEmpty = true;
    for(int i = sectionStartsFromLine; i<sectionEndsAtLine;++i) {
        if(formatString(lines[i]).size() != 0) {
            completelyEmpty = false;
            break;
        }
    }
    if(completelyEmpty) return true;
    
    vector<RuleUncompiled> rulesUncompiled;
    bool success = toUncompiledRule(lines, sectionStartsFromLine, sectionEndsAtLine,  rulesUncompiled, generatorSection, game, logger);
    if(!success) return false;
    
    if(!generatorSection) {
        game.rules.clear();
        success = toCompiledRule(rulesUncompiled, game.rules, game, logger);
    }
    else {
        game.generatorRules.clear();
        success = toCompiledRule(rulesUncompiled, game.generatorRules, game, logger);
    }
    if(!success) return false;
    return true;
}
