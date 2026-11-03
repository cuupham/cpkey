//
//  Engine.h
//  OpenKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#ifndef Engine_h
#define Engine_h

#include "DataType.h"
#include "Vietnamese.h"
#include "Macro.h"
#include "TextEdit.h"

// Layout of the switch-key setting: low byte and top byte hold the key, the middle bits hold the modifiers.
#define EMPTY_SWITCH_KEY      0xFEu
#define EMPTY_HOTKEY          ((EMPTY_SWITCH_KEY << 24) | EMPTY_SWITCH_KEY)
#define SWITCH_KEY_CTRL_MASK  0x100
#define SWITCH_KEY_ALT_MASK   0x200
#define SWITCH_KEY_WIN_MASK   0x400
#define SWITCH_KEY_SHIFT_MASK 0x800
#define SWITCH_KEY_BEEP_MASK  0x8000

#define SWITCH_KEY_MODIFIER_MASK  0x00FFFF00
#define GET_SWITCH_KEY(data)      ((data) & 0xFF)
#define SET_SWITCH_KEY(data, key) (((data) & SWITCH_KEY_MODIFIER_MASK) | (key) | ((unsigned int)(key) << 24))
#define HAS_CONTROL(data)         (((data) & SWITCH_KEY_CTRL_MASK) ? 1 : 0)
#define HAS_ALT(data)             (((data) & SWITCH_KEY_ALT_MASK) ? 1 : 0)
#define HAS_WIN(data)             (((data) & SWITCH_KEY_WIN_MASK) ? 1 : 0)
#define HAS_SHIFT(data)           (((data) & SWITCH_KEY_SHIFT_MASK) ? 1 : 0)
#define GET_BOOL(data)            ((data) ? 1 : 0)
#define HAS_BEEP(data)            ((data) & SWITCH_KEY_BEEP_MASK)

// Settings the application must define (see Settings.h). All are 0/1 flags except vInputType.
extern int vInputType; // vKeyInputType
extern int vCheckSpelling;
extern int vUseModernOrthography; // 1: oà, uý; 0: òa, úy
extern int vRestoreIfWrongSpelling; // needs vCheckSpelling
extern int vUseMacro;
extern int vUpperCaseFirstChar;
extern int vAllowConsonantZFWJ;

// Call first; returns the vKeyHookState the engine fills after each key.
void* vKeyInit();

// Converts an engine character to a real character.
uint32_t getCharacterCode(const uint32_t& data);

// Entry point for each key or mouse event. capsStatus: 1 shift, 2 caps lock.
// otherControlKey: Ctrl or Alt is held.
void vKeyHandleEvent(const vKeyEvent& event,
                     const vKeyEventState& state,
                     const uint16_t& data,
                     const uint8_t& capsStatus = 0,
                     const bool& otherControlKey = false);

// Starts a new word.
void startNewSession();

// Key handling while typing English: only the macro feature is active.
void vEnglishMode(const vKeyEventState& state, const uint16_t& data, const bool& isCaps, const bool& otherControlKey);

// Toggles spell checking, only when the user had it on.
void vTempOffSpellChecking();

// Re-reads vCheckSpelling after the setting changed.
void vSetCheckSpelling();

// Pauses the engine until the next word break; false resumes it.
void vTempOffEngine(const bool& off = true);

#endif /* Engine_h */
