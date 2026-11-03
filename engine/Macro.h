#ifndef Macro_h
#define Macro_h

#include <vector>
#include "DataType.h"

using namespace std;

// Looks up the text typed as abbreviation (key codes, CAPS_MASK for upper case). On a match fills
// macroContentCode with the characters to type instead. Implemented by the application.
bool findMacro(vector<Uint32>& key, vector<Uint32>& macroContentCode);

#endif
