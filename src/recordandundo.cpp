#include "recordandundo.h"

#include "game.h"
#include "global.h"
#include "logError.h"
#include "stringUtilities.h"
#include "visualsandide.h"

/*
template<typename T> ostream& operator<<(ostream& os, const vector<T> & v) {
    for(const auto & c : v)
        os << c << endl;
    return os;
}

template<typename T> istream& operator>>(istream& is, vector<T> & v) {
    size_t s;
    is >> s;
    for(size_t i=0;i<s;++i) {
        T t;
        is >> t;
        v.push_back(t);
    }
    return is;
}*/


ostream& operator<<(ostream& os, const Record & r) {
    os << "version " << gbl::version << endl;
    os << r.editorHistory.size() << endl;
    for(const auto & h : r.editorHistory) {
        os << h.first.first << endl;
        os << h.first.second << endl;
        os << get<0>(h.second) << endl;
        os << get<1>(h.second) << endl;
        os << get<2>(h.second).size() << endl;
        for(const string & str : get<2>(h.second)) os << str << endl;
        os << get<3>(h.second).size() << endl;
        for(const string & str : get<3>(h.second)) os << str << endl;
        os << get<4>(h.second).size() << endl;
        for(const string & str : get<4>(h.second)) os << str << endl;
        
        os << get<5>(h.second);

    }
    return os;
}
istream& operator>>(istream& is, Record & r) {
    string versionstr; is >> versionstr;
    int ver;
    is >> ver;
    if(gbl::version != ver) {
        cout << "Version mismatch: " << ver << "(file) and " << gbl::version << "(program)";
        return is;
    }
    
    //time_t ctime = chrono::system_clock::to_time_t(chrono::system_clock::now());
    //std::cout << "finished computation at " << std::ctime(&end_time)
    
    
    size_t s; is >> s;
    cout << "total number: " << s << endl;
    for(size_t i=0;i<s;++i) {
        //first
        pair<pair<uint64_t,string>, tuple<MODE_TYPE, Game, vector<string>, vector<string>, vector<string>, vector<vector<vector<bool> > > > > entry;
        is >> entry.first.first;
        is >> entry.first.second;
        unsigned int u; is >> u;  get<0>(entry.second) = static_cast<MODE_TYPE>(u);
        
        
        Game g;
        is >> g;
        
        size_t sv;
#define PRINTSTRS(number)\
        is >> sv;\
        for(size_t svi=0;svi<=sv;++svi) {\
            string str;\
            std::getline(is, str);\
            get<number>(entry.second).push_back(str);\
        }
        
        PRINTSTRS(2);
        cout << "apparent size " << sv << endl;
        PRINTSTRS(3);
        
        cout << "apparent size " << sv << endl;
        PRINTSTRS(4);
        
        cout << "apparent size " << sv << endl;
        
        vector<vector<vector<bool> > > vvvb;
        is >> vvvb;
        get<5>(entry.second) = vvvb;
        
        /*
        Game oldgame = g;
        auto successes = parseGame(get<3>(entry.second), get<4>(entry.second), g, logger::levelEdit, logger::generator);
        if(successes.first && !successes.second && get<0>(entry.second) != MODE_EXPLOITATION) {
            successes = parseGame(get<3>(entry.second), {"==========","TRANSFORM","==========","[] -> []"}, gbl::currentGame, logger::levelEdit, logger::generator);
        }
        if(!successes.first || !successes.second) g = oldgame;
        */
        get<1>(entry.second) = g;
        
        r.editorHistory.push_back(entry);
        //now get game by compiling it.
    }
    return is;
}


/*
 TODO: MAKE MODIFYTHINGY NOT AN EDITOR THINGY
*/

void pushEditorState(Record & r, string type) {
    cout << "trying to add to the undo list" << endl;
    for(char c : type) assert(!isspace(c));
    uint64_t time = 0;
#ifndef compiledWithoutOpenframeworks
    time = ofGetElapsedTimeMillis();
#endif
    auto p = make_pair(make_pair(time,type) , make_tuple( gbl::mode, gbl::currentGame, editor::ideString, editor::levelEditorString, editor::exploitationString, editor::modifyTable ) );
    if(r.editorHistory.size() == 0
       || get<0>(p.second) != get<0>(r.editorHistory.back().second)
       || get<1>(p.second).getTotalHash() != get<1>(r.editorHistory.back().second).getTotalHash()
       || get<2>(p.second) != get<2>(r.editorHistory.back().second)
       || get<3>(p.second) != get<3>(r.editorHistory.back().second)
       || get<4>(p.second) != get<4>(r.editorHistory.back().second)
       || get<5>(p.second) != get<5>(r.editorHistory.back().second)) {
        cout << "adding to the undo list" << endl;
        r.editorHistory.push_back( p );
    }
}


void undoEditorState(Record & r) {
    cerr << "undo level editor" << endl;
    //assert(editor::activeIDE == false);
    if(r.editorHistory.size() > 1) {
        r.editorHistory.pop_back();
    }
    gbl::mode = get<0>(r.editorHistory.back().second);
    gbl::currentGame = get<1>(r.editorHistory.back().second);
    editor::ideString = get<2>(r.editorHistory.back().second);
    editor::levelEditorString = get<3>(r.editorHistory.back().second);
    editor::exploitationString = get<4>(r.editorHistory.back().second);
    editor::modifyTable = get<5>(r.editorHistory.back().second);
}

static int lastLogIndex(string dirlocation) {
    ofDirectory dir;
    dir.open(dirlocation);
    int numFiles = dir.listDir();
    
    for (int i=0; i<numFiles; ++i) {
        cout << "Path at index [" << i << "] = " << dir.getPath(i) << endl;
    }
    return 0;
}

void exportRecordToFile(const Record & r, string location) {
    ofDirectory dir;
    dir.open(location);
    if(!dir.exists()){
        dir.create(true);
    }
    int numFiles = dir.listDir();
    cerr << "Contains " << numFiles << " files." << endl;
    int cmax = 0;
    for (int i=0; i<numFiles; ++i) {
        string str = dir.getPath(i);
        if(isInt(str.substr(12))) {
            cmax = MAX(cmax, stoi(str.substr(12)));
        }
        cerr << "Path at index [" << i << "] = " << dir.getPath(i) << endl;
    }
    ofstream ofs;
    ofs.open(location+"/"+to_string(cmax+1));
    ofs << r << endl;
    ofs.close();
}
void importRecordFromFile(Record & r, string location) {
    cerr << "importing from file " << location << endl;
    ofDirectory dir;
    dir.open(location);
    if(!dir.exists()){
        dir.create(true);
        return;
    }
    int numFiles = dir.listDir();
    int cmax = 0, maxlocation = -1;
    for (int i=0; i<numFiles; ++i) {
        cerr << dir.getPath(i) << endl;
        string str = dir.getPath(i);
        if(isInt(str.substr(12))) {
            if(cmax < stoi(str.substr(12))) maxlocation = i;
            cmax = MAX(cmax, stoi(str.substr(12)));
        }
    }
    if(maxlocation == -1) return;
    
    ifstream ifs;
    ifs.open(dir.getPath(maxlocation));
    if(ifs.good()) {
        ifs >> r;
        cerr << "done reading" << endl;
        pushEditorState(r,"importing");
    } else {
        cerr << "import stream " << location+"/"+dir.getPath(maxlocation) << " not good" << endl;
    }
    ifs.close();
}

