#pragma once

#define DEFAULT_SWITCH_STATUS 0xFE0009FE // Ctrl+Shift, no extra key

// Every user-facing setting persisted in CPKey.ini: X(name, default).
// This list is the single source for declaring, defining, loading and resetting them.
#define PERSISTED_SETTINGS(X) \
	X(vInputType, 0) \
	X(vCheckSpelling, 1) \
	X(vUseModernOrthography, 0) \
	X(vSwitchKeyStatus, DEFAULT_SWITCH_STATUS) \
	X(vRestoreIfWrongSpelling, 1) \
	X(vFixRecommendBrowser, 1) \
	X(vSendKeyStepByStep, 1) \
	X(vUpperCaseFirstChar, 0) \
	X(vTempOffSpelling, 0) \
	X(vAllowConsonantZFWJ, 0) \
	X(vUseGrayIcon, 0) \
	X(vRunWithWindows, 0) \
	X(vRunAsAdmin, 0) \
	X(vFixChromiumBrowser, 0) \
	X(vUseMacro, 1)

#define DECLARE_SETTING(name, value) extern int name;
PERSISTED_SETTINGS(DECLARE_SETTING)
