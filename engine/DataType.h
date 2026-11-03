//
//  DataType.h
//  OpenKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#ifndef DataType_h
#define DataType_h

#include <vector>

using namespace std;

#define MAX_BUFF 32

enum vKeyEvent {
    Keyboard,
    Mouse
};

enum vKeyEventState {
    KeyDown,
    KeyUp,
    MouseDown,
    MouseUp
};

enum vKeyInputType {
    vTelex = 0,
    vVNI,
    vSimpleTelex1,
    vSimpleTelex2
};

typedef unsigned char Byte;
typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;

enum HookCodeState {
    vDoNothing = 0,
    vWillProcess, // replace the typed word with charData
    vBreakWord,
    vRestore, // restore the keys typed before the word was transformed
    vReplaceMacro,
    vRestoreAndStartNewSession, // restore on an invalid word ended by a break character (, . ")
};

// Result of a key event, read by the application.
struct vKeyHookState {
    Byte code; // HookCodeState
    Byte backspaceCount;
    Byte newCharCount;

    // 1: word break, 2: delete key, 3: normal key, 4: do not send the empty character
    Byte extCode;

    Uint32 charData[MAX_BUFF]; // new characters, in reverse typing order

    vector<Uint32> macroKey; // keys typed as the abbreviation
    vector<Uint32> macroData; // characters typed instead
};

#include "platforms/win32.h"

//internal engine data
#define CAPS_MASK  0x10000
#define TONE_MASK  0x20000
#define TONEW_MASK 0x40000

/*
 * MARK MASK
 * 1: Dấu Sắc - á
 * 2: Dấu Huyền - à
 * 3: Dấu Hỏi - ả
 * 4: Dấu Ngã - ã
 * 5: dấu Nặng - ạ
 */
#define MARK1_MASK 0x80000
#define MARK2_MASK 0x100000
#define MARK3_MASK 0x200000
#define MARK4_MASK 0x400000
#define MARK5_MASK 0x800000

//for checking has mark or not
#define MARK_MASK 0xF80000

//mark and get first 16 bytes character
#define CHAR_MASK 0xFFFF

//Whether the data was created by a standalone key (W)
#define STANDALONE_MASK 0x1000000

//Whether the data is a character code (not a keyboard code)
#define CHAR_CODE_MASK 0x2000000

#define PURE_CHARACTER_MASK 0x80000000

//for special feature
#define END_CONSONANT_MASK   0x4000
#define CONSONANT_ALLOW_MASK 0x8000

//Utilities macro
#define IS_CONSONANT(keyCode) !(keyCode == KEY_A || keyCode == KEY_E || keyCode == KEY_U || keyCode == KEY_Y || keyCode == KEY_I || keyCode == KEY_O)
//#define IS_MARK_KEY(keyCode) (keyCode == KEY_S || keyCode == KEY_F || keyCode == KEY_R || keyCode == KEY_J || keyCode == KEY_X)
#define CHR(index) (Uint16) TypingWord[index]
#define IS_SPECIALKEY(keyCode)                                                                                                                                                                                                           \
    (vInputType == vTelex ? keyCode == KEY_W || keyCode == KEY_E || keyCode == KEY_R || keyCode == KEY_O || keyCode == KEY_LEFT_BRACKET ||                                                                                               \
                                keyCode == KEY_RIGHT_BRACKET || keyCode == KEY_A || keyCode == KEY_S || keyCode == KEY_D || keyCode == KEY_F || keyCode == KEY_J ||                                                                      \
                                keyCode == KEY_Z || keyCode == KEY_X || keyCode == KEY_W                                                                                                                                                 \
                          : (vInputType == vVNI ? keyCode == KEY_1 || keyCode == KEY_2 || keyCode == KEY_3 || keyCode == KEY_4 ||                                                                                                        \
                                                      keyCode == KEY_5 || keyCode == KEY_6 || keyCode == KEY_7 || keyCode == KEY_8 || keyCode == KEY_9 || keyCode == KEY_0                                                               \
                                                : (vInputType == vSimpleTelex1 ? keyCode == KEY_W || keyCode == KEY_E || keyCode == KEY_R || keyCode == KEY_O || keyCode == KEY_A || keyCode == KEY_S ||                                 \
                                                                                     keyCode == KEY_D || keyCode == KEY_F || keyCode == KEY_J || keyCode == KEY_Z || keyCode == KEY_X || keyCode == KEY_W                                \
                                                                               : (vInputType == vSimpleTelex2 ? keyCode == KEY_W || keyCode == KEY_E || keyCode == KEY_R || keyCode == KEY_O || keyCode == KEY_A || keyCode == KEY_S ||  \
                                                                                                                    keyCode == KEY_D || keyCode == KEY_F || keyCode == KEY_J || keyCode == KEY_Z || keyCode == KEY_X || keyCode == KEY_W \
                                                                                                              : false))))

#define IS_NUMBER_KEY(code) (code == KEY_1 || code == KEY_2 || code == KEY_3 || code == KEY_4 || code == KEY_5 || code == KEY_6 || code == KEY_7 || code == KEY_8 || code == KEY_9 || code == KEY_0)

#endif /* DataType_h */
