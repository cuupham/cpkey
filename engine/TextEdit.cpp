#include "TextEdit.h"
#include "Vietnamese.h"

TextEdit planTextEdit(const vKeyHookState& state) {
    TextEdit edit = { false, 0, {}, false, false };
    if (state.code == vReplaceMacro) {
        edit.consumed = true;
        edit.backspaces = state.backspaceCount;
        edit.chars = state.macroData;
        edit.retypeKey = true;
        return edit;
    }

    bool restores = state.code == vRestore || state.code == vRestoreAndStartNewSession;
    if (state.code != vWillProcess && !restores) {
        return edit;
    }
    edit.consumed = true;
    edit.backspaces = state.backspaceCount;
    if (state.newCharCount > 0 && state.newCharCount <= MAX_BUFF) {
        for (int i = state.newCharCount - 1; i >= 0; i--) {
            edit.chars.push_back(state.charData[i]);
        }
    }
    edit.retypeKey = restores;
    edit.startsNewSession = state.code == vRestoreAndStartNewSession;
    return edit;
}

int backspacesToSend(int count) {
    return count > 0 && count < MAX_BUFF ? count : 0;
}

uint16_t engineCharToUnicode(uint32_t data) {
    if (data & (CHAR_CODE_MASK | PURE_CHARACTER_MASK)) {
        return (uint16_t)data;
    }
    return keyCodeToCharacter(data);
}
