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

#ifndef LOBYTE
#define LOBYTE(data) (data & 0xFF)
#endif // !LOBYTE
#ifndef HIBYTE
#define HIBYTE(data) ((data >> 8) & 0xFF)
#endif // !HIBYTE

#define GET_SWITCH_KEY(data) (data & 0xFF)
#define HAS_CONTROL(data)    ((data & 0x100) ? 1 : 0)
#define HAS_OPTION(data)     ((data & 0x200) ? 1 : 0)
#define HAS_COMMAND(data)    ((data & 0x400) ? 1 : 0)
#define HAS_SHIFT(data)      ((data & 0x800) ? 1 : 0)
#define GET_BOOL(data)       (data ? 1 : 0)
#define HAS_BEEP(data)       (data & 0x8000)

//define these variable in your application
//API
/*
 * 0: Telex
 * 1: VNI
 */
extern int vInputType;

/**
 * 0: No
 * 1: Yes
 */
extern int vCheckSpelling;

/*
 * 0: òa, úy
 * 1: oà uý
*/
extern int vUseModernOrthography;

/**
 * Work together with vCheckSpelling
 * 0: No
 * 1: Yes
 *
 */
extern int vRestoreIfWrongSpelling;

/**
 * Macro on or off
 */
extern int vUseMacro;

/**
 * Auto write upper case character for first letter.
 * 0: No
 * 1: Yes
 */
extern int vUpperCaseFirstChar;

/**
 * Allow write word with consonant Z, F, W, J
 * 0: No
 * 1: Yes
 */
extern int vAllowConsonantZFWJ;

/**
 * Call this function first to receive data pointer
 */
void* vKeyInit();

/**
 * Convert engine character to real character
 */
Uint32 getCharacterCode(const Uint32& data);

/**
 * MAIN entry point for each key
 * event: mouse or keyboard event
 * state: additional state for event
 * data: key code
 * isCaps: caplock is on or shift key is pressing
 * otherControlKey: ctrl, option,... is pressing
 */
void vKeyHandleEvent(const vKeyEvent& event,
                     const vKeyEventState& state,
                     const Uint16& data,
                     const Uint8& capsStatus = 0,
                     const bool& otherControlKey = false);

/**
 * Start a new word
 */
void startNewSession();

/**
 * do some task in english mode (use for macro)
 */
void vEnglishMode(const vKeyEventState& state, const Uint16& data, const bool& isCaps, const bool& otherControlKey);

/**
 * temporarily turn off spell checking
 */
void vTempOffSpellChecking();

/**
 * reset spelling value
 */
void vSetCheckSpelling();

/**
 * temporarily turn off OpenKey engine
 */
void vTempOffEngine(const bool& off = true);

/**
 * some utils function
 */

#endif /* Engine_h */
