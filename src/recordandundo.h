#ifndef recordandundo_h
#define recordandundo_h

#include "macros.h"

struct Record {
    //entire editor history
    vector<pair<pair<uint64_t,string>, tuple<MODE_TYPE, Game, vector<string>, vector<string>, vector<string>, vector<vector<vector<bool> > > > > > editorHistory;
    
    //number of times solving, switching modes, etc.
    int solveIssue;
    int modeSwitching;
};

ostream& operator<<(ostream& os, const Record & r);
istream& operator>>(istream& os, Record & r);

extern void pushEditorState(Record & r, string type);
extern void undoEditorState(Record & r);

extern void exportRecordToFile(const Record & r, string location);
extern void importRecordFromFile(Record & r, string location);



#endif /* recordandundo_h */
