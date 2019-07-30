#ifndef logError_h
#define logError_h

#include "macros.h"

class Logger {
public:
    string lastErrorStr="";
    int lastErrorLine=-1;
    
    string lastWarningStr="";
    int lastWarningLine=-1;
    
    void logError(string errorMsg, int lineno);
    void logWarning(string errorMsg, int lineno);
    void reset();
};

namespace logger {
    extern Logger levelEdit;
    extern Logger generator;
}

#endif /* logError_h */
