#include "../engine/Engine.h"
#include "Suites.h"
#include "Test.h"

static vKeyHookState stateWith(uint8_t code, uint8_t backspaces, uint8_t newChars) {
    vKeyHookState state = {};
    state.code = code;
    state.backspaceCount = backspaces;
    state.newCharCount = newChars;
    return state;
}

static void testKeysTheEngineIgnoresPassThrough() {
    CHECK(!planTextEdit(stateWith(vDoNothing, 0, 0)).consumed);
    CHECK(!planTextEdit(stateWith(vBreakWord, 3, 3)).consumed);
}

static void testProcessedWordIsRetypedInTypingOrder() {
    vKeyHookState state = stateWith(vWillProcess, 2, 3);
    state.charData[0] = 10;
    state.charData[1] = 20;
    state.charData[2] = 30;

    TextEdit edit = planTextEdit(state);
    CHECK(edit.consumed);
    CHECK_EQ(edit.backspaces, 2);
    CHECK(edit.chars == vector<uint32_t>({ 30, 20, 10 }));
    CHECK(!edit.retypeKey);
    CHECK(!edit.startsNewSession);
}

static void testRestoreRetypesTheKey() {
    TextEdit restore = planTextEdit(stateWith(vRestore, 1, 1));
    CHECK(restore.consumed);
    CHECK(restore.retypeKey);
    CHECK(!restore.startsNewSession);

    TextEdit restoreAndStart = planTextEdit(stateWith(vRestoreAndStartNewSession, 1, 1));
    CHECK(restoreAndStart.consumed);
    CHECK(restoreAndStart.retypeKey);
    CHECK(restoreAndStart.startsNewSession);
}

static void testImplausibleCharCountTypesNothing() {
    CHECK(planTextEdit(stateWith(vWillProcess, 1, 0)).chars.empty());
    CHECK(planTextEdit(stateWith(vWillProcess, 1, MAX_BUFF + 1)).chars.empty());
    CHECK_EQ(planTextEdit(stateWith(vWillProcess, 1, MAX_BUFF)).chars.size(), MAX_BUFF);
}

static void testMacroReplacesTypedKeysWithItsText() {
    vKeyHookState state = stateWith(vReplaceMacro, 2, 0);
    state.macroData = { L'V' | PURE_CHARACTER_MASK, L'N' | PURE_CHARACTER_MASK };

    TextEdit edit = planTextEdit(state);
    CHECK(edit.consumed);
    CHECK_EQ(edit.backspaces, 2);
    CHECK(edit.chars == state.macroData);
    CHECK(edit.retypeKey);
    CHECK(!edit.startsNewSession);
}

static void testBackspaceLimit() {
    CHECK_EQ(backspacesToSend(-1), 0);
    CHECK_EQ(backspacesToSend(0), 0);
    CHECK_EQ(backspacesToSend(1), 1);
    CHECK_EQ(backspacesToSend(MAX_BUFF - 1), MAX_BUFF - 1);
    CHECK_EQ(backspacesToSend(MAX_BUFF), 0);
}

static void testEngineCharToUnicode() {
    CHECK_EQ(engineCharToUnicode(0x1EC7 | CHAR_CODE_MASK), 0x1EC7);
    CHECK_EQ(engineCharToUnicode(L'V' | PURE_CHARACTER_MASK), 'V');
    CHECK_EQ(engineCharToUnicode(KEY_A), 'a');
    CHECK_EQ(engineCharToUnicode(KEY_A | CAPS_MASK), 'A');
    CHECK_EQ(engineCharToUnicode(KEY_ENTER), 0);
}

void runTextEditTests() {
    testKeysTheEngineIgnoresPassThrough();
    testProcessedWordIsRetypedInTypingOrder();
    testRestoreRetypesTheKey();
    testImplausibleCharCountTypesNothing();
    testMacroReplacesTypedKeysWithItsText();
    testBackspaceLimit();
    testEngineCharToUnicode();
}
