//
//  Engine.cpp
//  OpenKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
#include <algorithm>
#include "Engine.h"
#include <string.h>
#include <list>
#include "Macro.h"

static vector<uint8_t> charKeyCodes = {
    KEY_BACKQUOTE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUALS,
    KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET, KEY_BACK_SLASH,
    KEY_SEMICOLON, KEY_QUOTE, KEY_COMMA, KEY_DOT, KEY_SLASH
};

static vector<uint8_t> breakKeyCodes = {
    KEY_ESC, KEY_TAB, KEY_ENTER, KEY_RETURN, KEY_LEFT, KEY_RIGHT, KEY_DOWN, KEY_UP, KEY_COMMA, KEY_DOT,
    KEY_SLASH, KEY_SEMICOLON, KEY_QUOTE, KEY_BACK_SLASH, KEY_MINUS, KEY_EQUALS, KEY_BACKQUOTE,
    VK_INSERT, VK_HOME, VK_END, VK_DELETE, VK_PRIOR, VK_NEXT, VK_SNAPSHOT, VK_PRINT, VK_SELECT, VK_HELP,
    VK_EXECUTE, VK_NUMLOCK, VK_SCROLL
};

static vector<uint8_t> macroBreakKeyCodes = {
    KEY_RETURN, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_SEMICOLON, KEY_QUOTE, KEY_BACK_SLASH, KEY_MINUS, KEY_EQUALS
};

static uint16_t ProcessingChar[][11] = {
    { KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z }, //Telex
    { KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0 }, //VNI
    { KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z }, //Simple Telex 1
    { KEY_S, KEY_F, KEY_R, KEY_X, KEY_J, KEY_A, KEY_O, KEY_E, KEY_W, KEY_D, KEY_Z } //Simple Telex 2
};

#define IS_KEY_Z(key)      (ProcessingChar[vInputType][10] == key)
#define IS_KEY_D(key)      (ProcessingChar[vInputType][9] == key)
#define IS_KEY_W(key)      ((vInputType != vVNI) ? ProcessingChar[vInputType][8] == key : (vInputType == vVNI ? (ProcessingChar[vInputType][8] == key || ProcessingChar[vInputType][7] == key) : false))
#define IS_KEY_DOUBLE(key) ((vInputType != vVNI) ? (ProcessingChar[vInputType][5] == key || ProcessingChar[vInputType][6] == key || ProcessingChar[vInputType][7] == key) : (vInputType == vVNI ? ProcessingChar[vInputType][6] == key : false))
#define IS_KEY_S(key)      (ProcessingChar[vInputType][0] == key)
#define IS_KEY_F(key)      (ProcessingChar[vInputType][1] == key)
#define IS_KEY_R(key)      (ProcessingChar[vInputType][2] == key)
#define IS_KEY_X(key)      (ProcessingChar[vInputType][3] == key)
#define IS_KEY_J(key)      (ProcessingChar[vInputType][4] == key)

#define IS_MARK_KEY(keyCode) (((vInputType != vVNI) && (keyCode == KEY_S || keyCode == KEY_F || keyCode == KEY_R || keyCode == KEY_J || keyCode == KEY_X)) || \
                              (vInputType == vVNI && (keyCode == KEY_1 || keyCode == KEY_2 || keyCode == KEY_3 || keyCode == KEY_5 || keyCode == KEY_4)))
#define IS_BRACKET_KEY(key)  (key == KEY_LEFT_BRACKET || key == KEY_RIGHT_BRACKET)

#define VSI        vowelStartIndex
#define VEI        vowelEndIndex
#define VWSM       vowelWillSetMark
#define hBPC       HookState.backspaceCount
#define hNCC       HookState.newCharCount
#define hCode      HookState.code
#define hExt       HookState.extCode
#define hData      HookState.charData
#define GET        getCharacterCode
#define hMacroKey  HookState.macroKey
#define hMacroData HookState.macroData

//Result handed to the application
static vKeyHookState HookState;

/**
 * data structure of each element in TypingWord (uint32_t)
 * first 2 byte is character code or key code.
 * bit 16: has caps or not
 * bit 17: has tone ^ or not
 * bit 18: has tone w or not
 * bit 19 - > 23: has mark or not (Sắc, huyền, hỏi, ngã, nặng)
 * bit 24: is standalone key? (w, [, ])
 * bit 25: is character code or keyboard code; 1: character code; 0: keycode
 */
static uint32_t TypingWord[MAX_BUFF];
static uint8_t wordLength = 0;
static vector<uint32_t> longWord; //save the word when wordLength >= MAX_BUFF
static list<vector<uint32_t>> wordHistory; //snapshots of the word, so it can be restored and edited after a delete
static vector<uint32_t> historyEntry;

/**
 * Use for restore key if invalid word
 */
static uint32_t KeyStates[MAX_BUFF];
static uint8_t stateCount = 0;

static bool tempDisableKey = false;
static bool isCorrect = false;
static bool isChanged = false;
static uint8_t vowelCount = 0;
static uint8_t vowelStartIndex = 0;
static uint8_t vowelEndIndex = 0;
static uint8_t vowelWillSetMark = 0;
static bool keyIsCaps = false;
static int spaceCount = 0;
static bool macroHandled = false; //set when a macro was already replaced for the current word
static uint8_t upperCaseStatus = 0; //for Write upper case for the first letter; 2: will upper case
static bool isCharKeyCode;
static vector<uint32_t> specialChars;
static bool spellCheckingWasOn;
static bool engineTempOff = false;

//function prototype
static void findAndCalculateVowel(const bool& forGrammar = false);
static void insertMark(const uint32_t& markMask, const bool& canModifyFlag = true);

void* vKeyInit() {
    wordLength = 0;
    stateCount = 0;
    spellCheckingWasOn = vCheckSpelling;
    historyEntry.clear();
    wordHistory.clear();
    longWord.clear();
    return &HookState;
}

static bool isWordBreak(const vKeyEvent& event, const vKeyEventState& state, const uint16_t& data) {
    if (event == vKeyEvent::Mouse)
        return true;
    for (int i = 0; i < breakKeyCodes.size(); i++) {
        if (breakKeyCodes[i] == data) {
            return true;
        }
    }
    return false;
}

static bool isMacroBreakCode(const int& data) {
    for (int i = 0; i < macroBreakKeyCodes.size(); i++) {
        if (macroBreakKeyCodes[i] == data) {
            return true;
        }
    }
    return false;
}

static void setKeyData(const uint8_t& index, const uint16_t& keyCode, const bool& isCaps) {
    if (index >= MAX_BUFF)
        return;
    TypingWord[index] = keyCode | (isCaps ? CAPS_MASK : 0);
}

static bool spellingOK = false;
static bool spellingFlag = false;
static bool spellingVowelOK = false;
static uint8_t spellingEnd = 0;

static void checkSpelling(const bool& forceCheckVowel = false) {
    spellingOK = false;
    spellingVowelOK = true;
    spellingEnd = wordLength;

    if (wordLength > 0 && CHR(wordLength - 1) == KEY_RIGHT_BRACKET) {
        spellingEnd = wordLength - 1;
    }

    if (spellingEnd > 0) {
        int j = 0;
        //Check first consonant
        if (IS_CONSONANT(CHR(0))) {
            for (int i = 0; i < consonantTable.size(); i++) {
                spellingFlag = false;
                if (spellingEnd < consonantTable[i].size())
                    spellingFlag = true;
                for (j = 0; j < consonantTable[i].size(); j++) {
                    if (spellingEnd > j &&
                        consonantTable[i][j] != CHR(j) &&
                        (consonantTable[i][j] & ~(vAllowConsonantZFWJ ? CONSONANT_ALLOW_MASK : 0)) != CHR(j)) {
                        spellingFlag = true;
                        break;
                    }
                }
                if (spellingFlag)
                    continue;

                break;
            }
        }

        if (j == spellingEnd) { //for "d" case
            spellingOK = true;
        }

        //check next vowel
        int k = j;
        VSI = k;
        //case "que't"
        if (CHR(VSI) == KEY_U && k > 0 && k < spellingEnd - 1 && CHR(VSI - 1) == KEY_Q) {
            k = k + 1;
            j = k;
            VSI = k;
        } else if (wordLength >= 2 && CHR(0) == KEY_G && CHR(1) == KEY_I && IS_CONSONANT(CHR(2))) {
            VSI = k = j = 1; //case "gìn"
        }
        for (int l = 0; l < 3; l++) {
            if (k < spellingEnd && !IS_CONSONANT(CHR(k))) {
                k++;
                VEI = k;
            }
        }
        if (k > j) { //has vowel,
            spellingVowelOK = false;
            //check correct combined vowel
            if (k - j > 1 && forceCheckVowel) {
                vector<vector<uint32_t>>& vowelSet = vowelCombinations[CHR(j)];
                for (int l = 0; l < vowelSet.size(); l++) {
                    spellingFlag = false;
                    int ii;
                    for (ii = 1; ii < vowelSet[l].size(); ii++) {
                        if (j + ii - 1 < spellingEnd && vowelSet[l][ii] != ((CHR(j + ii - 1) | (TypingWord[j + ii - 1] & TONEW_MASK) | (TypingWord[j + ii - 1] & TONE_MASK)))) {
                            spellingFlag = true;
                            break;
                        }
                    }
                    if (spellingFlag || (k < spellingEnd && !vowelSet[l][0]) || (j + ii - 1 < spellingEnd && !IS_CONSONANT(CHR(j + ii - 1))))
                        continue;

                    spellingVowelOK = true;
                    break;
                }
            } else if (!IS_CONSONANT(CHR(j))) {
                spellingVowelOK = true;
            }

            //continue check last consonant
            for (int ii = 0; ii < endConsonantTable.size(); ii++) {
                spellingFlag = false;

                for (j = 0; j < endConsonantTable[ii].size(); j++) {
                    if (spellingEnd > k + j &&
                        endConsonantTable[ii][j] != CHR(k + j)) {
                        spellingFlag = true;
                        break;
                    }
                }
                if (spellingFlag)
                    continue;

                if (k + j >= spellingEnd) {
                    spellingOK = true;
                    break;
                }
            }

            //limit: end consonant "ch", "t" can not use with "~", "`", "?"
            if (spellingOK) {
                if (wordLength >= 3 && CHR(wordLength - 1) == KEY_H && CHR(wordLength - 2) == KEY_C && !((TypingWord[wordLength - 3] & MARK1_MASK) || (TypingWord[wordLength - 3] & MARK5_MASK) || !(TypingWord[wordLength - 3] & MARK_MASK))) {
                    spellingOK = false;
                } else if (wordLength >= 2 && CHR(wordLength - 1) == KEY_T && !((TypingWord[wordLength - 2] & MARK1_MASK) || (TypingWord[wordLength - 2] & MARK5_MASK) || !(TypingWord[wordLength - 2] & MARK_MASK))) {
                    spellingOK = false;
                }
            }
        }
    } else {
        spellingOK = true;
    }
    tempDisableKey = !(spellingOK && spellingVowelOK);
}

static void checkGrammar(const int& deltaBackSpace) {
    if (wordLength <= 1 || wordLength >= MAX_BUFF)
        return;

    findAndCalculateVowel(true);
    if (vowelCount == 0)
        return;

    bool isCheckedGrammar = false;

    int firstVowel = VSI;

    //if N key for case: "thuơn", "ưoi", "ưom", "ưoc"
    if (wordLength >= 3) {
        for (int i = wordLength - 1; i >= 0; i--) {
            if (CHR(i) == KEY_N || CHR(i) == KEY_C || CHR(i) == KEY_I ||
                CHR(i) == KEY_M || CHR(i) == KEY_P || CHR(i) == KEY_T) {
                if (i - 2 >= 0 && CHR(i - 1) == KEY_O && CHR(i - 2) == KEY_U) {
                    if ((TypingWord[i - 1] & TONEW_MASK) ^ (TypingWord[i - 2] & TONEW_MASK)) {
                        TypingWord[i - 2] |= TONEW_MASK;
                        TypingWord[i - 1] |= TONEW_MASK;
                        isCheckedGrammar = true;
                        break;
                    }
                }
            }
        }
    }

    //check mark
    if (wordLength >= 2) {
        for (int i = firstVowel; i <= VEI; i++) {
            if (TypingWord[i] & MARK_MASK) {
                uint32_t mark = TypingWord[i] & MARK_MASK;
                TypingWord[i] &= ~MARK_MASK;
                insertMark(mark, false);
                if (i != vowelWillSetMark)
                    isCheckedGrammar = true;
                break;
            }
        }
    }

    //re-arrange data to sendback
    if (isCheckedGrammar) {
        if (hCode == vDoNothing)
            hCode = vWillProcess;
        hBPC = 0;

        for (int i = wordLength - 1; i >= firstVowel; i--) {
            hBPC++;
            hData[wordLength - 1 - i] = GET(TypingWord[i]);
        }
        hNCC = hBPC;
        hBPC += deltaBackSpace;
        hExt = 4;
    }
}

static void insertKey(const uint16_t& keyCode, const bool& isCaps, const bool& isCheckSpelling = true) {
    if (wordLength >= MAX_BUFF) {
        longWord.push_back(TypingWord[0]); //save long word
        //left shift
        for (int i = 0; i < MAX_BUFF - 1; i++) {
            TypingWord[i] = TypingWord[i + 1];
        }
        setKeyData(wordLength - 1, keyCode, isCaps);
    } else {
        setKeyData(wordLength++, keyCode, isCaps);
    }

    if (vCheckSpelling && isCheckSpelling)
        checkSpelling();

    //allow d after consonant
    if (keyCode == KEY_D && wordLength - 2 >= 0 && IS_CONSONANT(CHR(wordLength - 2)))
        tempDisableKey = false;
}

static void insertState(const uint16_t& keyCode, const bool& isCaps) {
    if (stateCount >= MAX_BUFF) {
        //left shift
        for (int i = 0; i < MAX_BUFF - 1; i++) {
            KeyStates[i] = KeyStates[i + 1];
        }
        KeyStates[stateCount - 1] = keyCode | (isCaps ? CAPS_MASK : 0);
    } else {
        KeyStates[stateCount++] = keyCode | (isCaps ? CAPS_MASK : 0);
    }
}

static void saveWord() {
    //save word history
    if (hCode != vReplaceMacro) {
        if (wordLength > 0) {
            if (longWord.size() > 0) { //save long word first
                historyEntry.clear();
                for (int i = 0; i < longWord.size(); i++) {
                    if (i != 0 && i % MAX_BUFF == 0) { //save if overflow
                        wordHistory.push_back(historyEntry);
                        historyEntry.clear();
                    }
                    historyEntry.push_back(longWord[i]);
                }
                wordHistory.push_back(historyEntry);
                longWord.clear();
            }

            //save current word
            historyEntry.clear();
            for (int i = 0; i < wordLength; i++) {
                historyEntry.push_back(TypingWord[i]);
            }
            wordHistory.push_back(historyEntry);
        }
    } else { //save macro words
        historyEntry.clear();
        for (int i = 0; i < hMacroData.size(); i++) {
            if (i != 0 && i % MAX_BUFF == 0) { //break if overflow
                wordHistory.push_back(historyEntry);
                historyEntry.clear();
            }
            historyEntry.push_back(hMacroData[i]);
        }
        wordHistory.push_back(historyEntry);
    }
}

static void saveWord(const uint32_t& keyCode, const int& count) {
    historyEntry.clear();
    for (int i = 0; i < count; i++) {
        historyEntry.push_back(keyCode);
    }
    wordHistory.push_back(historyEntry);
}

static void saveSpecialChar() {
    historyEntry.clear();
    for (int i = 0; i < specialChars.size(); i++) {
        historyEntry.push_back(specialChars[i]);
    }
    wordHistory.push_back(historyEntry);
    specialChars.clear();
}

static void restoreLastTypingState() {
    if (wordHistory.size() > 0) {
        historyEntry = wordHistory.back();
        wordHistory.pop_back();
        if (historyEntry.size() > 0) {
            if (historyEntry[0] == KEY_SPACE) {
                spaceCount = (int)historyEntry.size();
                wordLength = 0;
            } else if (std::find(charKeyCodes.begin(), charKeyCodes.end(), (uint16_t)historyEntry[0]) != charKeyCodes.end()) {
                wordLength = 0;
                specialChars = historyEntry;
                checkSpelling();
            } else {
                for (int i = 0; i < historyEntry.size(); i++) {
                    TypingWord[i] = historyEntry[i];
                }
                wordLength = (uint8_t)historyEntry.size();
            }
        }
    }
}

void startNewSession() {
    wordLength = 0;
    hBPC = 0;
    hNCC = 0;
    tempDisableKey = false;
    stateCount = 0;
    macroHandled = false;
    longWord.clear();
}

static void checkCorrectVowel(vector<vector<uint16_t>>& charset, int index, const uint16_t& markKey) {
    //ignore "qu" case
    if (wordLength >= 2 && CHR(wordLength - 1) == KEY_U && CHR(wordLength - 2) == KEY_Q) {
        isCorrect = false;
        return;
    }
    int k = wordLength - 1;
    for (int j = (int)charset[index].size() - 1; j >= 0; j--) {
        if (charset[index][j] != CHR(k)) {
            isCorrect = false;
            return;
        }
        k--;
        if (k < 0)
            break;
    }

    //limit mark for end consonant: "C", "T"
    if (isCorrect && charset[index].size() > 1 && (IS_KEY_F(markKey) || IS_KEY_X(markKey) || IS_KEY_R(markKey))) {
        if (charset[index][1] == KEY_C || charset[index][1] == KEY_T) {
            isCorrect = false;
        } else if (charset[index].size() > 2 && (charset[index][2] == KEY_T)) {
            isCorrect = false;
        }
    }

    if (isCorrect && k >= 0) {
        if (CHR(k) == CHR(k + 1)) {
            isCorrect = false;
        }
    }
}

uint32_t getCharacterCode(const uint32_t& data) {
    int capsElem = (data & CAPS_MASK) ? 0 : 1;
    int key = data & CHAR_MASK;
    if (data & MARK_MASK) { //has mark
        int markElem = -2;
        switch (data & MARK_MASK) {
            case MARK1_MASK:
                markElem = 0;
                break;
            case MARK2_MASK:
                markElem = 2;
                break;
            case MARK3_MASK:
                markElem = 4;
                break;
            case MARK4_MASK:
                markElem = 6;
                break;
            case MARK5_MASK:
                markElem = 8;
                break;
        }
        markElem += capsElem;

        switch (key) {
            case KEY_A:
            case KEY_O:
            case KEY_U:
            case KEY_E:
                if ((data & TONE_MASK) == 0 && (data & TONEW_MASK) == 0)
                    markElem += 4;
                break;
        }

        if (data & TONE_MASK) {
            key |= TONE_MASK;
        } else if (data & TONEW_MASK) {
            key |= TONEW_MASK;
        }
        if (unicodeTable.find(key) == unicodeTable.end())
            return data; //not found

        return unicodeTable[key][markElem] | CHAR_CODE_MASK;
    } else { //doesn't has mark
        if (unicodeTable.find(key) == unicodeTable.end())
            return data; //not found

        if (data & TONE_MASK) {
            return unicodeTable[key][capsElem] | CHAR_CODE_MASK;
        } else if (data & TONEW_MASK) {
            return unicodeTable[key][capsElem + 2] | CHAR_CODE_MASK;
        } else {
            return data; //not found
        }
    }

    return 0;
}

static void findAndCalculateVowel(const bool& forGrammar) {
    vowelCount = 0;
    VSI = VEI = 0;
    for (int i = wordLength - 1; i >= 0; i--) {
        if (IS_CONSONANT(CHR(i))) {
            if (vowelCount > 0)
                break;
        } else { //is vowel
            if (vowelCount == 0)
                VEI = i;
            if (!forGrammar) {
                if ((i - 1 >= 0 && (CHR(i) == KEY_I && CHR(i - 1) == KEY_G)) ||
                    (i - 1 >= 0 && (CHR(i) == KEY_U && CHR(i - 1) == KEY_Q))) {
                    break;
                }
            }
            VSI = i;
            vowelCount++;
        }
    }
    //don't count "u" at "q u" as a vowel
    if (VSI - 1 >= 0 && CHR(VSI) == KEY_U && CHR(VSI - 1) == KEY_Q) {
        VSI++;
        vowelCount--;
    }
}

static void removeMark() {
    findAndCalculateVowel(true);
    isChanged = false;
    if (wordLength > 0) {
        for (int i = VSI; i <= VEI; i++) {
            if (TypingWord[i] & MARK_MASK) {
                TypingWord[i] &= ~MARK_MASK;
                isChanged = true;
            }
        }
    }
    if (isChanged) {
        hCode = vWillProcess;
        hBPC = 0;

        for (int i = wordLength - 1; i >= VSI; i--) {
            hBPC++;
            hData[wordLength - 1 - i] = GET(TypingWord[i]);
        }
        hNCC = hBPC;
    } else {
        hCode = vDoNothing;
    }
}

static bool canHasEndConsonant() {
    vector<vector<uint32_t>>& vo = vowelCombinations[CHR(VSI)];
    for (int row = 0; row < vo.size(); row++) {
        int position = VSI;
        int column = 1;
        for (; column < vo[row].size(); column++) {
            if (position > VEI || ((CHR(position) | (TypingWord[position] & TONE_MASK) | (TypingWord[position] & TONEW_MASK)) != vo[row][column])) {
                break;
            }
            position++;
        }
        if (column >= vo[row].size()) {
            return vo[row][0] == 1;
        }
    }
    return false;
}

static void handleModernMark() {
    //default
    VWSM = VEI;
    hBPC = (wordLength - VEI);

    //rule 2
    if (vowelCount == 3 && ((CHR(VSI) == KEY_O && CHR(VSI + 1) == KEY_A && CHR(VSI + 2) == KEY_I) ||
                            (CHR(VSI) == KEY_U && CHR(VSI + 1) == KEY_Y && CHR(VSI + 2) == KEY_U) ||
                            (CHR(VSI) == KEY_O && CHR(VSI + 1) == KEY_E && CHR(VSI + 2) == KEY_O) ||
                            (CHR(VSI) == KEY_U && CHR(VSI + 1) == KEY_Y && CHR(VSI + 2) == KEY_A))) {
        VWSM = VSI + 1;
        hBPC = wordLength - VWSM;
    } else if ((CHR(VSI) == KEY_O && CHR(VSI + 1) == KEY_I) ||
               (CHR(VSI) == KEY_A && CHR(VSI + 1) == KEY_I) ||
               (CHR(VSI) == KEY_U && CHR(VSI + 1) == KEY_I)) {

        VWSM = VSI;
        hBPC = wordLength - VWSM;
    } else if (CHR(VEI - 1) == KEY_A && CHR(VEI) == KEY_Y) {
        VWSM = VEI - 1;
        hBPC = (wordLength - VEI) + 1;
    } else if (CHR(VSI) == KEY_U && CHR(VSI + 1) == KEY_O) {
        VWSM = VSI + 1;
        hBPC = wordLength - VWSM;
    } else if (CHR(VSI + 1) == KEY_O || CHR(VSI + 1) == KEY_U) {
        VWSM = VEI - 1;
        hBPC = (wordLength - VEI) + 1;
    } else if (CHR(VSI) == KEY_O || CHR(VSI) == KEY_U) {
        VWSM = VEI;
        hBPC = (wordLength - VEI);
    }

    //rule 3.1
    if ((CHR(VSI) == KEY_I && (TypingWord[VSI + 1] & (KEY_E | TONE_MASK))) ||
        (CHR(VSI) == KEY_Y && (TypingWord[VSI + 1] & (KEY_E | TONE_MASK))) ||
        (CHR(VSI) == KEY_U && (TypingWord[VSI + 1] == (KEY_O | TONE_MASK))) ||
        ((TypingWord[VSI] == (KEY_U | TONEW_MASK)) && (TypingWord[VSI + 1] == (KEY_O | TONEW_MASK)))) {

        if (VSI + 2 < wordLength) {
            if (CHR(VSI + 2) == KEY_P || CHR(VSI + 2) == KEY_T ||
                CHR(VSI + 2) == KEY_M || CHR(VSI + 2) == KEY_N ||
                CHR(VSI + 2) == KEY_O || CHR(VSI + 2) == KEY_U ||
                CHR(VSI + 2) == KEY_I || CHR(VSI + 2) == KEY_C ||
                (VSI + 3 < wordLength && CHR(VSI + 2) == KEY_C && CHR(VSI + 2) == KEY_H) ||
                (VSI + 3 < wordLength && CHR(VSI + 2) == KEY_N && CHR(VSI + 2) == KEY_H) ||
                (VSI + 3 < wordLength && CHR(VSI + 2) == KEY_N && CHR(VSI + 2) == KEY_G)) {

                VWSM = VSI + 1;
                hBPC = wordLength - VWSM;
            } else {
                VWSM = VSI;
                hBPC = wordLength - VWSM;
            }
        } else {
            VWSM = VSI;
            hBPC = wordLength - VWSM;
        }
    }
    //rule 3.2
    else if ((CHR(VSI) == KEY_I && (CHR(VSI) == KEY_A)) ||
             (CHR(VSI) == KEY_Y && (CHR(VSI) == KEY_A)) ||
             (CHR(VSI) == KEY_U && (CHR(VSI) == KEY_A)) ||
             (CHR(VSI) == KEY_U && (TypingWord[VSI + 1] == (KEY_U | TONEW_MASK)))) {

        VWSM = VSI;
        hBPC = wordLength - VWSM;
    }

    //rule 4
    if (vowelCount == 2) {
        if (((CHR(VSI) == KEY_I) && (CHR(VSI + 1) == KEY_A)) ||
            ((CHR(VSI) == KEY_I) && (CHR(VSI + 1) == KEY_U)) ||
            ((CHR(VSI) == KEY_I) && (CHR(VSI + 1) == KEY_O))) {

            if (VSI == 0 || (CHR(VSI - 1) != KEY_G)) { //no G before
                VWSM = VSI;
                hBPC = wordLength - VWSM;
            } else {
                VWSM = VSI + 1;
                hBPC = wordLength - VWSM;
            }
        } else if ((CHR(VSI) == KEY_U) && (CHR(VSI + 1) == KEY_A)) {
            if (VSI == 0 || (CHR(VSI - 1) != KEY_Q)) { //no Q before
                if (VEI + 1 >= wordLength || !canHasEndConsonant()) {
                    VWSM = VSI;
                    hBPC = wordLength - VWSM;
                }
            } else {
                VWSM = VSI + 1;
                hBPC = wordLength - VWSM;
            }
        } else if ((CHR(VSI) == KEY_O) && (CHR(VSI + 1) == KEY_O)) { //case "thoong"
            VWSM = VEI;
            hBPC = wordLength - VWSM;
        }
    }
}

static void handleOldMark() {
    //default
    if (vowelCount == 0 && CHR(VEI) == KEY_I)
        VWSM = VEI;
    else
        VWSM = VSI;
    hBPC = (wordLength - VWSM);

    //rule 2
    if (vowelCount == 3 || (VEI + 1 < wordLength && IS_CONSONANT(CHR(VEI + 1)) && canHasEndConsonant())) {
        VWSM = VSI + 1;
        hBPC = wordLength - VWSM;
    }

    //rule 3
    for (int i = VSI; i <= VEI; i++) {
        if ((CHR(i) == KEY_E && TypingWord[i] & TONE_MASK) || (CHR(i) == KEY_O && TypingWord[i] & TONEW_MASK)) {
            VWSM = i;
            hBPC = wordLength - VWSM;
            break;
        }
    }

    hNCC = hBPC;
}

static void insertMark(const uint32_t& markMask, const bool& canModifyFlag) {
    vowelCount = 0;

    if (canModifyFlag)
        hCode = vWillProcess;
    hBPC = hNCC = 0;

    findAndCalculateVowel();
    VWSM = 0;

    //detect mark position
    if (vowelCount == 1) {
        VWSM = VEI;
        hBPC = (wordLength - VEI);
    } else { //vowel = 2 or 3
        if (vUseModernOrthography == 0)
            handleOldMark();
        else
            handleModernMark();
        if (TypingWord[VEI] & TONE_MASK || TypingWord[VEI] & TONEW_MASK)
            vowelWillSetMark = VEI;
    }

    //send data
    int dataIndex = wordLength - 1 - VSI;
    //if duplicate same mark -> restore
    if (TypingWord[VWSM] & markMask) {

        TypingWord[VWSM] &= ~MARK_MASK;
        if (canModifyFlag)
            hCode = vRestore;
        for (int i = VSI; i < wordLength; i++) {
            TypingWord[i] &= ~MARK_MASK;
            hData[dataIndex--] = GET(TypingWord[i]);
        }
        tempDisableKey = true;
    } else {
        //remove other mark
        TypingWord[VWSM] &= ~MARK_MASK;

        //add mark
        TypingWord[VWSM] |= markMask;
        for (int i = VSI; i < wordLength; i++) {
            if (i != VWSM) { //remove mark for other vowel
                TypingWord[i] &= ~MARK_MASK;
            }
            hData[dataIndex--] = GET(TypingWord[i]);
        }

        hBPC = wordLength - VSI;
    }
    hNCC = hBPC;
}

static void insertD(const uint16_t& data, const bool& isCaps) {
    hCode = vWillProcess;
    hBPC = 0;
    for (int i = wordLength - 1; i >= 0; i--) {
        hBPC++;
        if (CHR(i) == KEY_D) { //reverse unicode char
            if (TypingWord[i] & TONE_MASK) {
                //restore and disable temporary
                hCode = vRestore;
                TypingWord[i] &= ~TONE_MASK;
                hData[wordLength - 1 - i] = TypingWord[i];
                tempDisableKey = true;
                break;
            } else {
                TypingWord[i] |= TONE_MASK;
                hData[wordLength - 1 - i] = GET(TypingWord[i]);
            }
            break;
        } else { //keep the old char
            hData[wordLength - 1 - i] = GET(TypingWord[i]);
        }
    }
    hNCC = hBPC;
}

static void insertAOE(const uint16_t& data, const bool& isCaps) {
    findAndCalculateVowel();

    //remove W tone
    for (int i = VSI; i <= VEI; i++) {
        TypingWord[i] &= ~TONEW_MASK;
    }

    hCode = vWillProcess;
    hBPC = 0;

    for (int i = wordLength - 1; i >= 0; i--) {
        hBPC++;
        if (CHR(i) == data) { //reverse unicode char
            if (TypingWord[i] & TONE_MASK) {
                //restore and disable temporary
                hCode = vRestore;
                TypingWord[i] &= ~TONE_MASK;
                hData[wordLength - 1 - i] = TypingWord[i];
                if (data != KEY_O) //case thoòng
                    tempDisableKey = true;
                break;
            } else {
                TypingWord[i] |= TONE_MASK;
                if (!IS_KEY_D(data))
                    TypingWord[i] &= ~TONEW_MASK;
                hData[wordLength - 1 - i] = GET(TypingWord[i]);
            }
            break;
        } else { //keep the old char
            hData[wordLength - 1 - i] = GET(TypingWord[i]);
        }
    }
    hNCC = hBPC;
}

static void insertW(const uint16_t& data, const bool& isCaps) {
    findAndCalculateVowel();

    //remove ^ tone
    for (int i = VSI; i <= VEI; i++) {
        TypingWord[i] &= ~TONE_MASK;
    }

    if (vowelCount > 1) {
        hBPC = wordLength - VSI;
        hNCC = hBPC;

        if (((TypingWord[VSI] & TONEW_MASK) && (TypingWord[VSI + 1] & TONEW_MASK)) ||
            ((TypingWord[VSI] & TONEW_MASK) && CHR(VSI + 1) == KEY_I) ||
            ((TypingWord[VSI] & TONEW_MASK) && CHR(VSI + 1) == KEY_A)) {
            //restore and disable temporary
            hCode = vRestore;

            for (int i = VSI; i < wordLength; i++) {
                TypingWord[i] &= ~TONEW_MASK;
                hData[wordLength - 1 - i] = GET(TypingWord[i]) & ~STANDALONE_MASK;
            }
            tempDisableKey = true;
        } else {
            hCode = vWillProcess;

            if ((CHR(VSI) == KEY_U && CHR(VSI + 1) == KEY_O)) {
                if (VSI - 2 >= 0 && TypingWord[VSI - 2] == KEY_T && TypingWord[VSI - 1] == KEY_H) {
                    TypingWord[VSI + 1] |= TONEW_MASK;
                    if (VSI + 2 < wordLength && CHR(VSI + 2) == KEY_N) {
                        TypingWord[VSI] |= TONEW_MASK;
                    }
                } else if (VSI - 1 >= 0 && TypingWord[VSI - 1] == KEY_Q) {
                    TypingWord[VSI + 1] |= TONEW_MASK;
                } else {
                    TypingWord[VSI] |= TONEW_MASK;
                    TypingWord[VSI + 1] |= TONEW_MASK;
                }
            } else if ((CHR(VSI) == KEY_U && CHR(VSI + 1) == KEY_A) ||
                       (CHR(VSI) == KEY_U && CHR(VSI + 1) == KEY_I) ||
                       (CHR(VSI) == KEY_U && CHR(VSI + 1) == KEY_U) ||
                       (CHR(VSI) == KEY_O && CHR(VSI + 1) == KEY_I)) {
                TypingWord[VSI] |= TONEW_MASK;
            } else if ((CHR(VSI) == KEY_I && CHR(VSI + 1) == KEY_O) ||
                       (CHR(VSI) == KEY_O && CHR(VSI + 1) == KEY_A)) {
                TypingWord[VSI + 1] |= TONEW_MASK;
            } else {
                //don't do anything
                tempDisableKey = true;
                isChanged = false;
                hCode = vDoNothing;
            }

            for (int i = VSI; i < wordLength; i++) {
                hData[wordLength - 1 - i] = GET(TypingWord[i]);
            }
        }

        return;
    }

    hCode = vWillProcess;
    hBPC = 0;

    for (int i = wordLength - 1; i >= 0; i--) {
        if (i < VSI)
            break;
        hBPC++;
        switch (CHR(i)) {
            case KEY_A:
            case KEY_U:
            case KEY_O:
                if (TypingWord[i] & TONEW_MASK) {
                    //restore and disable temporary
                    if (TypingWord[i] & STANDALONE_MASK) {
                        hCode = vWillProcess;
                        if (CHR(i) == KEY_U) {
                            TypingWord[i] = KEY_W | ((TypingWord[i] & CAPS_MASK) ? CAPS_MASK : 0);
                        } else if (CHR(i) == KEY_O) {
                            hCode = vRestore;
                            TypingWord[i] = KEY_O | ((TypingWord[i] & CAPS_MASK) ? CAPS_MASK : 0);
                        }
                        hData[wordLength - 1 - i] = TypingWord[i];
                    } else {
                        hCode = vRestore;
                        TypingWord[i] &= ~TONEW_MASK;
                        hData[wordLength - 1 - i] = TypingWord[i];
                    }

                    tempDisableKey = true;
                } else {
                    TypingWord[i] |= TONEW_MASK;
                    TypingWord[i] &= ~TONE_MASK;
                    hData[wordLength - 1 - i] = GET(TypingWord[i]);
                }
                break;

            default:
                hData[wordLength - 1 - i] = GET(TypingWord[i]);
                break;
        }
    }
    hNCC = hBPC;
}

static void reverseLastStandaloneChar(const uint32_t& keyCode, const bool& isCaps) {
    hCode = vWillProcess;
    hBPC = 0;
    hNCC = 1;
    hExt = 4;
    TypingWord[wordLength - 1] = (keyCode | TONEW_MASK | STANDALONE_MASK | (isCaps ? CAPS_MASK : 0));
    hData[0] = GET(TypingWord[wordLength - 1]);
}

static void checkForStandaloneChar(const uint16_t& data, const bool& isCaps, const uint32_t& keyWillReverse) {
    if (CHR(wordLength - 1) == keyWillReverse && TypingWord[wordLength - 1] & TONEW_MASK) {
        hCode = vWillProcess;
        hBPC = 1;
        hNCC = 1;
        TypingWord[wordLength - 1] = data | (isCaps ? CAPS_MASK : 0);
        hData[0] = GET(TypingWord[wordLength - 1]);
        return;
    }

    //check standalone w -> ư

    if (wordLength > 0 && CHR(wordLength - 1) == KEY_U && keyWillReverse == KEY_O) {
        insertKey(keyWillReverse, isCaps);
        reverseLastStandaloneChar(keyWillReverse, isCaps);
        return;
    }

    if (wordLength == 0) { //zero char
        insertKey(data, isCaps, false);
        reverseLastStandaloneChar(keyWillReverse, isCaps);
        return;
    } else if (wordLength == 1) { //1 char
        for (int i = 0; i < standaloneWBad.size(); i++) {
            if (CHR(0) == standaloneWBad[i]) {
                insertKey(data, isCaps);
                return;
            }
        }
        insertKey(data, isCaps, false);
        reverseLastStandaloneChar(keyWillReverse, isCaps);
        return;
    } else if (wordLength == 2) {
        for (int i = 0; i < doubleWAllowed.size(); i++) {
            if (CHR(0) == doubleWAllowed[i][0] && CHR(1) == doubleWAllowed[i][1]) {
                insertKey(data, isCaps, false);
                reverseLastStandaloneChar(keyWillReverse, isCaps);
                return;
            }
        }
        insertKey(data, isCaps);
        return;
    }

    insertKey(data, isCaps);
}

static void upperCaseFirstCharacter() {
    if (!(TypingWord[0] & CAPS_MASK)) {
        hCode = vWillProcess;
        hBPC = 0;
        hNCC = 1;
        TypingWord[0] |= CAPS_MASK;
        hData[0] = GET(TypingWord[0]);
        upperCaseStatus = 0;
        if (vUseMacro)
            hMacroKey[0] |= CAPS_MASK;
    }
}

static void handleMainKey(const uint16_t& data, const bool& isCaps) {
    //if is Z key, remove mark
    if (IS_KEY_Z(data)) {
        removeMark();
        if (!isChanged) {
            insertKey(data, isCaps);
        }
        return;
    }

    if (data == KEY_LEFT_BRACKET) { //standalone key [
        checkForStandaloneChar(data, isCaps, KEY_O);
        return;
    }

    if (data == KEY_RIGHT_BRACKET) { //standalone key }
        checkForStandaloneChar(data, isCaps, KEY_U);
        return;
    }

    //if is D key
    if (IS_KEY_D(data)) {
        isCorrect = false;
        isChanged = false;
        for (int i = 0; i < consonantsForD.size(); i++) {
            if (wordLength < consonantsForD[i].size())
                continue;
            isCorrect = true;
            checkCorrectVowel(consonantsForD, i, data);

            //allow d after consonant
            if (!isCorrect && wordLength - 2 >= 0 && CHR(wordLength - 1) == KEY_D && IS_CONSONANT(CHR(wordLength - 2))) {
                isCorrect = true;
            }
            if (isCorrect) {
                isChanged = true;
                insertD(data, isCaps);
                break;
            }
        }

        if (!isChanged) {
            insertKey(data, isCaps);
        }
        return;
    }

    //if is mark key
    if (IS_MARK_KEY(data)) {
        for (auto& vowelEntry : vowelsForMark) {
            vector<vector<uint16_t>>& charset = vowelEntry.second;
            isCorrect = false;
            isChanged = false;
            for (int i = 0; i < charset.size(); i++) {
                if (wordLength < charset[i].size())
                    continue;
                isCorrect = true;
                checkCorrectVowel(charset, i, data);

                if (isCorrect) {
                    isChanged = true;
                    if (IS_KEY_S(data))
                        insertMark(MARK1_MASK);
                    else if (IS_KEY_F(data))
                        insertMark(MARK2_MASK);
                    else if (IS_KEY_R(data))
                        insertMark(MARK3_MASK);
                    else if (IS_KEY_X(data))
                        insertMark(MARK4_MASK);
                    else if (IS_KEY_J(data))
                        insertMark(MARK5_MASK);
                    break;
                }
            }

            if (isCorrect) {
                break;
            }
        }

        if (!isChanged) {
            insertKey(data, isCaps);
        }

        return;
    }

    //check Vowel
    if (vInputType == vVNI) {
        for (int i = wordLength - 1; i >= 0; i--) {
            if (CHR(i) == KEY_O || CHR(i) == KEY_A || CHR(i) == KEY_E) {
                VEI = i;
                break;
            }
        }
    }

    uint16_t keyForAEO = ((vInputType != vVNI) ? data : ((data == KEY_7 || data == KEY_8 ? KEY_W : (data == KEY_6 ? TypingWord[VEI] : data))));
    vector<vector<uint16_t>>& charset = vowelByKey[keyForAEO];
    isCorrect = false;
    isChanged = false;
    for (int i = 0; i < charset.size(); i++) {
        if (wordLength < charset[i].size())
            continue;
        isCorrect = true;
        checkCorrectVowel(charset, i, data);

        if (isCorrect) {
            isChanged = true;
            if (IS_KEY_DOUBLE(data)) {
                insertAOE(keyForAEO, isCaps);
            } else if (IS_KEY_W(data)) {
                if (vInputType == vVNI) {
                    for (int j = wordLength - 1; j >= 0; j--) {
                        if (CHR(j) == KEY_O || CHR(j) == KEY_U || CHR(j) == KEY_A || CHR(j) == KEY_E) {
                            VEI = j;
                            break;
                        }
                    }
                    if ((data == KEY_7 && CHR(VEI) == KEY_A && (VEI - 1 >= 0 ? CHR(VEI - 1) != KEY_U : true)) || (data == KEY_8 && (CHR(VEI) == KEY_O || CHR(VEI) == KEY_U)))
                        break;
                }
                insertW(keyForAEO, isCaps);
            }
            break;
        }
    }

    if (!isChanged) {
        if (data == KEY_W && vInputType != vSimpleTelex1) {
            checkForStandaloneChar(data, isCaps, KEY_U);
        } else {
            insertKey(data, isCaps);
        }
    }
}

static bool checkRestoreIfWrongSpelling(const int& handleCode) {
    for (int i = 0; i < wordLength; i++) {
        if (!IS_CONSONANT(CHR(i)) &&
            (TypingWord[i] & MARK_MASK || TypingWord[i] & TONE_MASK || TypingWord[i] & TONEW_MASK)) {

            hCode = handleCode;
            hBPC = wordLength;
            hNCC = stateCount;
            for (int j = 0; j < stateCount; j++) {
                TypingWord[j] = KeyStates[j];
                hData[stateCount - 1 - j] = TypingWord[j];
            }
            wordLength = stateCount;
            return true;
        }
    }
    return false;
}

void vTempOffSpellChecking() {
    if (spellCheckingWasOn) {
        vCheckSpelling = vCheckSpelling ? 0 : 1;
    }
}

void vSetCheckSpelling() {
    spellCheckingWasOn = vCheckSpelling;
}

void vTempOffEngine(const bool& off) {
    engineTempOff = off;
}

/*==========================================================================================================*/

void vEnglishMode(const vKeyEventState& state, const uint16_t& data, const bool& isCaps, const bool& otherControlKey) {
    hCode = vDoNothing;
    if (state == vKeyEventState::MouseDown || (otherControlKey && !isCaps)) {
        hMacroKey.clear();
        engineTempOff = false;
    } else if (data == KEY_SPACE) {
        if (!macroHandled && findMacro(hMacroKey, hMacroData)) {
            hCode = vReplaceMacro;
            hBPC = (uint8_t)hMacroKey.size();
        }
        hMacroKey.clear();
        engineTempOff = false;
    } else if (data == KEY_DELETE) {
        if (hMacroKey.size() > 0) {
            hMacroKey.pop_back();
        } else {
            engineTempOff = false;
        }
    } else {
        if (isWordBreak(vKeyEvent::Keyboard, state, data) &&
            std::find(charKeyCodes.begin(), charKeyCodes.end(), data) == charKeyCodes.end()) {
            hMacroKey.clear();
            engineTempOff = false;
        } else {
            if (!engineTempOff)
                hMacroKey.push_back(data | (isCaps ? CAPS_MASK : 0));
        }
    }
}

void vKeyHandleEvent(const vKeyEvent& event,
                     const vKeyEventState& state,
                     const uint16_t& data,
                     const uint8_t& capsStatus,
                     const bool& otherControlKey) {
    keyIsCaps = (capsStatus == 1 || //shift
                 capsStatus == 2); //caps lock
    if ((IS_NUMBER_KEY(data) && capsStatus == 1) || otherControlKey || isWordBreak(event, state, data) || (wordLength == 0 && IS_NUMBER_KEY(data))) {
        hCode = vDoNothing;
        hBPC = 0;
        hNCC = 0;
        hExt = 1; //word break

        //check macro feature
        if (vUseMacro && isMacroBreakCode(data) && !macroHandled && findMacro(hMacroKey, hMacroData)) {
            hCode = vReplaceMacro;
            hBPC = (uint8_t)hMacroKey.size();
            macroHandled = true;
        } else if (vRestoreIfWrongSpelling && isWordBreak(event, state, data)) { //restore key if wrong spelling with break-key
            if (!tempDisableKey && vCheckSpelling) {
                checkSpelling(true); //force check spelling
            }
            if (tempDisableKey && !checkRestoreIfWrongSpelling(vRestoreAndStartNewSession)) {
                hCode = vDoNothing;
            }
        }

        isCharKeyCode = state == KeyDown && std::find(charKeyCodes.begin(), charKeyCodes.end(), data) != charKeyCodes.end();
        if (!isCharKeyCode) { //clear all line cache
            specialChars.clear();
            wordHistory.clear();
        } else { //check and save current word
            if (spaceCount > 0) {
                saveWord(KEY_SPACE, spaceCount);
                spaceCount = 0;
            } else {
                saveWord();
            }
            specialChars.push_back(data | (keyIsCaps ? CAPS_MASK : 0));
            hExt = 3; //normal word
        }

        if (hCode == vDoNothing) {
            startNewSession();
            vCheckSpelling = spellCheckingWasOn;
            engineTempOff = false;
        } else if (hCode == vReplaceMacro) {
            wordLength = 0;
        }

        //insert key for macro function
        if (vUseMacro) {
            if (isCharKeyCode) {
                hMacroKey.push_back(data | (keyIsCaps ? CAPS_MASK : 0));
            } else {
                hMacroKey.clear();
            }
        }

        if (vUpperCaseFirstChar) {
            if (data == KEY_DOT)
                upperCaseStatus = 1;
            else if (data == KEY_ENTER || data == KEY_RETURN)
                upperCaseStatus = 2;
            else
                upperCaseStatus = 0;
        }
    } else if (data == KEY_SPACE) {
        if (!tempDisableKey && vCheckSpelling) {
            checkSpelling(true); //force check spelling
        }
        if (vUseMacro && !macroHandled && findMacro(hMacroKey, hMacroData)) { //macro
            hCode = vReplaceMacro;
            hBPC = (uint8_t)hMacroKey.size();
            spaceCount++;
            macroHandled = true;
        } else if (vRestoreIfWrongSpelling && tempDisableKey && !macroHandled) { //restore key if wrong spelling
            if (!checkRestoreIfWrongSpelling(vRestore)) {
                hCode = vDoNothing;
            }
            spaceCount++;
        } else { //do nothing with SPACE KEY
            hCode = vDoNothing;
            spaceCount++;
        }
        if (vUseMacro) {
            hMacroKey.clear();
        }
        if (vUpperCaseFirstChar && upperCaseStatus == 1) {
            upperCaseStatus = 2;
        }
        //save word
        if (spaceCount == 1) {
            if (specialChars.size() > 0) {
                saveSpecialChar();
            } else {
                saveWord();
            }
        }
        vCheckSpelling = spellCheckingWasOn;
        engineTempOff = false;
    } else if (data == KEY_DELETE) {
        hCode = vDoNothing;
        hExt = 2; //delete
        if (specialChars.size() > 0) {
            specialChars.pop_back();
            if (specialChars.size() == 0) {
                restoreLastTypingState();
            }
        } else if (spaceCount > 0) { //previous char is space
            spaceCount--;
            if (spaceCount == 0) { //restore word
                restoreLastTypingState();
            }
        } else {
            if (stateCount > 0) {
                stateCount--;
            }
            if (wordLength > 0) {
                wordLength--;
                if (longWord.size() > 0) {
                    //right shift
                    for (int i = MAX_BUFF - 1; i > 0; i--) {
                        TypingWord[i] = TypingWord[i - 1];
                    }
                    TypingWord[0] = longWord.back();
                    longWord.pop_back();
                    wordLength++;
                }
                if (vCheckSpelling)
                    checkSpelling();
            }
            if (vUseMacro && hMacroKey.size() > 0) {
                hMacroKey.pop_back();
            }

            hBPC = 0;
            hNCC = 0;
            hExt = 2; //delete key
            if (wordLength == 0) {
                startNewSession();
                specialChars.clear();
                restoreLastTypingState();
            } else { //continue check grammar
                checkGrammar(1);
            }
        }
    } else { //START AND CHECK KEY
        if (engineTempOff) {
            hCode = vDoNothing;
            hExt = 3;
            return;
        }
        if (spaceCount > 0) {
            hBPC = 0;
            hNCC = 0;
            hExt = 0;
            startNewSession();
            //continue save space
            saveWord(KEY_SPACE, spaceCount);
            spaceCount = 0;
        } else if (specialChars.size() > 0) {
            saveSpecialChar();
        }

        insertState(data, keyIsCaps); //save state

        if (!IS_SPECIALKEY(data) || tempDisableKey) { //do nothing
            hCode = vDoNothing;
            hBPC = 0;
            hNCC = 0;
            hExt = 3; //normal key
            insertKey(data, keyIsCaps);
        } else { //check and update key
            //restore state
            hCode = vDoNothing;
            hExt = 3; //normal key
            handleMainKey(data, keyIsCaps);
        }

        if (!IS_KEY_D(data)) {
            if (hCode == vDoNothing) {
                checkGrammar(-1);
            } else {
                checkGrammar(0);
            }
        }

        if (hCode == vRestore) {
            insertKey(data, keyIsCaps);
            stateCount--;
        }

        //insert or replace key for macro feature
        if (vUseMacro) {
            if (hCode == vDoNothing) {
                hMacroKey.push_back(data | (keyIsCaps ? CAPS_MASK : 0));
            } else if (hCode == vWillProcess || hCode == vRestore) {
                for (int i = 0; i < hBPC; i++) {
                    if (hMacroKey.size() > 0) {
                        hMacroKey.pop_back();
                    }
                }
                for (int i = wordLength - hBPC; i < hNCC + (wordLength - hBPC); i++) {
                    hMacroKey.push_back(TypingWord[i]);
                }
            }
        }

        if (vUpperCaseFirstChar) {
            if (wordLength == 1 && upperCaseStatus == 2) {
                upperCaseFirstCharacter();
            }
            upperCaseStatus = 0;
        }

        //case [ ]
        if (IS_BRACKET_KEY(data) && ((IS_BRACKET_KEY((uint16_t)hData[0])) || vInputType == vSimpleTelex1 || vInputType == vSimpleTelex2)) {
            if (wordLength - (hCode == vWillProcess ? hBPC : 0) > 0) {
                wordLength--;
                saveWord();
            }
            wordLength = 0;
            tempDisableKey = false;
            stateCount = 0;
            hExt = 3;
            specialChars.push_back(data | (keyIsCaps ? CAPS_MASK : 0));
        }
    }
}
