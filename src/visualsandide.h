#ifndef visualsandide_h
#define visualsandide_h

#include "macros.h"

#ifndef compiledWithoutOpenframeworks
namespace editor {
    extern bool activeIDE;
    extern bool generatorIDE;
    extern int showGenerate;
    extern float ideFactor; //how much of the screen should be IDE
    extern float offsetIDEY;
    extern int selectedLevelIDE;
    extern vector<string> ideString;
    extern vector<string> levelEditorString;
    extern vector<string> exploitationString;
    extern pair<bool, bool> successes;
    
    extern MODE_TYPE previousMenuMode;
    
    extern int selectedBlock;
    extern string selectedBlockStr;
    extern int selectedExploitationTool;
    
    extern vector<vector<vector<bool> > > modifyTable;    
}

extern void initEditor(vector<string>);
extern void switchToLeftEditor(MODE_TYPE newmode, const string reason);
extern void switchToRightEditor(MODE_TYPE newmode);

extern void refreshIDEStrings();

extern void readjustIDEOffset();
extern void ideKeyPressed(int key, bool isSuperKey);

extern void displayLevelEditor();
extern void displayPlayMode();

extern void displayLevel(const vvvs & state, const Game & game, int offsetX, int offsetY, int width, int height, int min_field_size);
#endif

#endif /* visualsandide_h */
