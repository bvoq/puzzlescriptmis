#include "logError.h"

namespace logger {
    Logger levelEdit;
    Logger generator;
}


void Logger::logError(string errorMsg, int lineno) {
    lastErrorStr = errorMsg;
    lastErrorLine = lineno;
    cerr << "Parse error [" << lineno << "] " << ": " << endl;
    cerr << errorMsg << endl;
}


void Logger::logWarning(string errorMsg, int lineno) {
    lastWarningStr = errorMsg;
    lastWarningLine = lineno;
    cerr << "Parse warning [" << lineno << "] " << ": "  << endl;
    cerr << errorMsg << endl;
}

void Logger::reset() {
    lastErrorLine = -1;
    lastErrorStr = "";
    lastWarningLine = -1;
    lastWarningStr = "";
}
