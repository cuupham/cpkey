#ifndef TextEdit_h
#define TextEdit_h

#include "DataType.h"
#include <vector>

using namespace std;

// How the engine's answer to one key changes the focused text field. The application applies it with
// real input; the tests apply it to a string.
struct TextEdit {
    bool consumed; // false: the key goes through untouched
    int backspaces; // characters to erase first; run it through backspacesToSend
    vector<uint32_t> chars; // typed after erasing, in typing order; see engineCharToUnicode
    bool retypeKey; // type the pressed key again after chars
    bool startsNewSession; // call startNewSession() once the edit is applied
};

TextEdit planTextEdit(const vKeyHookState& state);

// An erase count outside (0, MAX_BUFF) is not trusted and erases nothing.
int backspacesToSend(int count);

// UTF-16 character of an engine character (finished character or key code); 0 for a key without one.
uint16_t engineCharToUnicode(uint32_t data);

#endif
