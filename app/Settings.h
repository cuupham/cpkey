#pragma once
#include "../engine/Engine.h"

#define DEFAULT_SWITCH_STATUS (EMPTY_HOTKEY | SWITCH_KEY_CTRL_MASK | SWITCH_KEY_SHIFT_MASK) // Ctrl+Shift, no extra key

// Every user-facing setting persisted in CPKey.ini: X(name, default).
// This list is the single source for declaring, defining, loading and resetting them.
#define PERSISTED_SETTINGS(X)                  \
	X(vInputType, vTelex)                      \
	X(vCheckSpelling, 1)                       \
	X(vUseModernOrthography, 0)                \
	X(vSwitchKeyStatus, DEFAULT_SWITCH_STATUS) \
	X(vRestoreIfWrongSpelling, 1)              \
	X(vFixRecommendBrowser, 1)                 \
	X(vSendKeyStepByStep, 1)                   \
	X(vUpperCaseFirstChar, 0)                  \
	X(vTempOffSpelling, 0)                     \
	X(vAllowConsonantZFWJ, 0)                  \
	X(vUseGrayIcon, 0)                         \
	X(vRunAsAdmin, 0)                          \
	X(vFixChromiumBrowser, 0)                  \
	X(vUseMacro, 1)

#define DECLARE_SETTING(name, value) extern int name;
PERSISTED_SETTINGS(DECLARE_SETTING)

// Mirrors the OS autostart registration, which is its only record; never stored in the INI.
// Os::registerRunOnStartup keeps it in step.
extern int vRunWithWindows;
