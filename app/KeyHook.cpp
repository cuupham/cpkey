#include "common.h"
#include "App.h"
#include "AppRules.h"
#include "QuickTyping.h"
#include "Settings.h"

#define IMC_GETOPENSTATUS    0x0005
#define IME_QUERY_TIMEOUT_MS 50

// Modifier state bits. Their numeric order matters: hotkey detection compares whole values.
#define MASK_SHIFT   0x01
#define MASK_CONTROL 0x02
#define MASK_ALT     0x04
#define MASK_CAPITAL 0x08
#define MASK_NUMLOCK 0x10
#define MASK_WIN     0x20
#define MASK_SCROLL  0x40

unsigned short lastKeyCode = 0;
int vLanguage = LANGUAGE_VIETNAMESE;

static const vector<string> CHROMIUM_BROWSERS = { "chrome.exe", "brave.exe", "msedge.exe" };

static HHOOK keyboardHook;
static HHOOK mouseHook;
static HWINEVENTHOOK foregroundHook;
static vKeyHookState* engineState;

static uint32_t modifiers = 0, lastModifiers = 0;
static bool isModifierKey = false;
static bool justUsedHotKey = false;
static uint16_t keyCode = 0;

// The language the user picked outside any app rule; restored when leaving a ruled app.
static int baseLanguage = LANGUAGE_VIETNAMESE;
static bool ruleActive = false;
static string ruleExe;

static INPUT backspaceEvents[2];
static INPUT keyEvents[2];

static LRESULT CALLBACK keyboardHookProcess(int nCode, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK mouseHookProcess(int nCode, WPARAM wParam, LPARAM lParam);
static VOID CALLBACK foregroundEventProcess(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD);

/*---------------------- sending input ----------------------*/

static void fillKeyEvent(INPUT& input, uint16_t virtualKey, bool press, DWORD flags = 0) {
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = press ? flags : flags | KEYEVENTF_KEYUP;
	input.ki.wVk = virtualKey;
	input.ki.wScan = 0;
	input.ki.time = 0;
	input.ki.dwExtraInfo = 1; // marks our own events so the hook ignores them
}

static void fillUnicodeEvent(INPUT& input, uint16_t character, bool press) {
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = 0;
	input.ki.wScan = character;
	input.ki.time = 0;
	input.ki.dwFlags = (press ? 0 : KEYEVENTF_KEYUP) | KEYEVENTF_UNICODE;
	input.ki.dwExtraInfo = 1;
}

static void sendVirtualKey(uint16_t virtualKey) {
	fillKeyEvent(keyEvents[0], virtualKey, true);
	fillKeyEvent(keyEvents[1], virtualKey, false);
	SendInput(2, keyEvents, sizeof(INPUT));
}

static void sendUnicode(uint16_t character) {
	fillUnicodeEvent(keyEvents[0], character, true);
	fillUnicodeEvent(keyEvents[1], character, false);
	SendInput(2, keyEvents, sizeof(INPUT));
}

// Presses `modifier`, taps `key`, releases `modifier`.
static void sendCombo(uint16_t modifier, uint16_t key, DWORD modifierFlags = 0, DWORD keyFlags = 0) {
	fillKeyEvent(keyEvents[0], modifier, true, modifierFlags);
	SendInput(1, keyEvents, sizeof(INPUT));

	fillKeyEvent(keyEvents[0], key, true, keyFlags);
	fillKeyEvent(keyEvents[1], key, false, keyFlags);
	SendInput(2, keyEvents, sizeof(INPUT));

	fillKeyEvent(keyEvents[0], modifier, false, modifierFlags);
	SendInput(1, keyEvents, sizeof(INPUT));
}

// `data` is an engine character: a finished character or a key code.
static void sendKeyCode(uint32_t data) {
	if (uint16_t character = engineCharToUnicode(data)) {
		sendUnicode(character);
	} else {
		sendVirtualKey((uint16_t)data);
	}
}

static void sendBackspaces(int count) {
	for (int i = 0; i < count; i++) {
		SendInput(2, backspaceEvents, sizeof(INPUT));
	}
}

// A narrow no-break space that stops browser address bars from auto-completing the word being fixed.
static void sendEmptyCharacter() {
	sendUnicode(0x202F);
}

// Pastes the new text through the clipboard (Shift+Insert) instead of typing it key by key.
static void sendByClipboard(const TextEdit& edit) {
	vector<WCHAR> text;
	for (uint32_t data : edit.chars) {
		text.push_back(engineCharToUnicode(data));
	}

	// A restore re-adds the key that triggered it; a control key (Tab, arrows...) is sent after the paste.
	bool sendControlKey = false;
	if (edit.retypeKey) {
		if (keyCodeToCharacter(keyCode)) {
			bool caps = (modifiers & MASK_SHIFT) || (modifiers & MASK_CAPITAL);
			text.push_back(keyCodeToCharacter(keyCode | (caps ? CAPS_MASK : 0)));
		} else {
			sendControlKey = true;
		}
	}
	if (edit.startsNewSession) {
		startNewSession();
	}

	text.push_back(0);
	Os::setClipboardText(text.data(), (int)text.size(), CF_UNICODETEXT);
	sendCombo(KEY_LEFT_SHIFT, VK_INSERT, 0, KEYEVENTF_EXTENDEDKEY);
	if (sendControlKey) {
		sendKeyCode(keyCode);
	}
}

// Erases the abbreviation just typed, types its replacement, then passes on the key that ended the word.
static void applyQuickTyping() {
	TextEdit edit = planTextEdit(*engineState);
	int backspaces = edit.backspaces;
	if (vFixRecommendBrowser) {
		sendEmptyCharacter();
		backspaces++;
	}
	sendBackspaces(backspacesToSend(backspaces));
	for (uint32_t data : edit.chars) {
		sendKeyCode(data);
	}
	sendKeyCode(keyCode | (modifiers & MASK_SHIFT ? CAPS_MASK : 0));
}

/*---------------------- language and app rules ----------------------*/

void applyAppRule() {
	const string& exe = Os::getForegroundExecutableName();
	if (exe == ruleExe)
		return;
	ruleExe = exe;

	int rule = getAppRule(exe);
	ruleActive = rule != RULE_NONE;
	vLanguage = ruleActive ? rule : baseLanguage;
	App::getInstance()->refreshUi();
}

void reapplyAppRule() {
	ruleExe.clear();
	applyAppRule();
}

bool isLanguageLocked() {
	return ruleActive;
}

int getBaseLanguage() {
	return baseLanguage;
}

void setBaseLanguage(int language) {
	baseLanguage = language;
	Os::setIntSetting(_T("vLanguage"), language);
	if (!ruleActive) {
		vLanguage = language;
	}
	App::getInstance()->refreshUi();
	startNewSession();
}

bool toggleLanguage() {
	if (ruleActive) {
		return false;
	}
	setBaseLanguage(vLanguage ? LANGUAGE_ENGLISH : LANGUAGE_VIETNAMESE);
	return true;
}

/*---------------------- hook life cycle ----------------------*/

static uint32_t readModifierState() {
	uint32_t state = 0;
	if (GetKeyState(VK_LSHIFT) < 0 || GetKeyState(VK_RSHIFT) < 0) state |= MASK_SHIFT;
	if (GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_RCONTROL) < 0) state |= MASK_CONTROL;
	if (GetKeyState(VK_LMENU) < 0 || GetKeyState(VK_RMENU) < 0) state |= MASK_ALT;
	if (GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0) state |= MASK_WIN;
	if (GetKeyState(VK_NUMLOCK) < 0) state |= MASK_NUMLOCK;
	if (GetKeyState(VK_CAPITAL) == 1) state |= MASK_CAPITAL;
	if (GetKeyState(VK_SCROLL) < 0) state |= MASK_SCROLL;
	return state;
}

void keyHookInit() {
#define LOAD_SETTING(name, value) APP_GET_DATA(name, value);
	PERSISTED_SETTINGS(LOAD_SETTING)
	APP_GET_DATA(vLanguage, LANGUAGE_VIETNAMESE);
	baseLanguage = vLanguage;
	if (vRunWithWindows) {
		Os::registerRunOnStartup(true); // refreshes the path if the exe was moved
	}

	engineState = (vKeyHookState*)vKeyInit();
	fillKeyEvent(backspaceEvents[0], VK_BACK, true);
	fillKeyEvent(backspaceEvents[1], VK_BACK, false);
	modifiers = readModifierState();

	loadAppRules();
	loadQuickTyping();

	HINSTANCE hInstance = GetModuleHandle(NULL);
	keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProcess, hInstance, 0);
	mouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseHookProcess, hInstance, 0);
	foregroundHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, foregroundEventProcess, 0, 0,
	                                 WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
}

void keyHookFree() {
	UnhookWindowsHookEx(mouseHook);
	UnhookWindowsHookEx(keyboardHook);
	UnhookWinEvent(foregroundHook);
}

/*---------------------- keyboard hook ----------------------*/

static uint32_t modifierBit(uint16_t virtualKey) {
	switch (virtualKey) {
		case VK_LSHIFT:
		case VK_RSHIFT: return MASK_SHIFT;
		case VK_LCONTROL:
		case VK_RCONTROL: return MASK_CONTROL;
		case VK_LMENU:
		case VK_RMENU: return MASK_ALT;
		case VK_LWIN:
		case VK_RWIN: return MASK_WIN;
		case VK_NUMLOCK: return MASK_NUMLOCK;
		case VK_SCROLL: return MASK_SCROLL;
	}
	return 0;
}

static bool hasControlModifier() {
	return (modifiers & MASK_ALT) || (modifiers & MASK_CONTROL);
}

// 0 none, 1 shift, 2 caps lock (shift and caps lock cancel out).
static uint8_t capsStatus() {
	bool shift = modifiers & MASK_SHIFT, caps = modifiers & MASK_CAPITAL;
	return (shift && caps) ? 0 : (shift ? 1 : (caps ? 2 : 0));
}

static bool matchesHotKey(int hotKeyData, bool checkKeyCode) {
	if ((hotKeyData & ~SWITCH_KEY_BEEP_MASK) == EMPTY_HOTKEY)
		return false;
	if (HAS_CONTROL(hotKeyData) ^ GET_BOOL(lastModifiers & MASK_CONTROL))
		return false;
	if (HAS_ALT(hotKeyData) ^ GET_BOOL(lastModifiers & MASK_ALT))
		return false;
	if (HAS_WIN(hotKeyData) ^ GET_BOOL(lastModifiers & MASK_WIN))
		return false;
	if (HAS_SHIFT(hotKeyData) ^ GET_BOOL(lastModifiers & MASK_SHIFT))
		return false;
	return !checkKeyCode || GET_SWITCH_KEY(hotKeyData) == keyCode;
}

static bool matchesSwitchKey() {
	return matchesHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != EMPTY_SWITCH_KEY);
}

static void switchLanguage() {
	if (toggleLanguage() && HAS_BEEP(vSwitchKeyStatus))
		MessageBeep(MB_OK);
}

// An open IME (Japanese, Chinese...) owns the keyboard. The wait is bounded so a hung window can never stall the hook.
static bool isImeOpen() {
	HWND ime = ImmGetDefaultIMEWnd(GetForegroundWindow());
	DWORD_PTR open = 0;
	return ime &&
	       SendMessageTimeout(ime, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, IME_QUERY_TIMEOUT_MS, &open) &&
	       open;
}

// Without the Vietnamese engine only quick typing is active.
static bool handleEnglishKey(bool isKeyDown) {
	if (!vUseMacro || !isKeyDown)
		return false;
	vEnglishMode(vKeyEventState::KeyDown, keyCode, (modifiers & MASK_SHIFT) || (modifiers & MASK_CAPITAL), hasControlModifier());
	if (engineState->code != vReplaceMacro)
		return false;
	applyQuickTyping();
	return true;
}

// Sends the engine's result to the app. Returns true when the key was consumed.
static bool applyEngineResult() {
	if (engineState->code == vReplaceMacro) {
		applyQuickTyping();
		return true;
	}
	TextEdit edit = planTextEdit(*engineState);
	if (!edit.consumed) {
		return false;
	}
	int backspaces = edit.backspaces;

	// Address bars and Excel auto-complete the word and duplicate letters after the backspaces.
	if (vFixRecommendBrowser && engineState->extCode != 4) {
		if (vFixChromiumBrowser &&
		    std::find(CHROMIUM_BROWSERS.begin(), CHROMIUM_BROWSERS.end(), Os::getLastExecutableName()) != CHROMIUM_BROWSERS.end()) {
			sendCombo(KEY_LEFT_SHIFT, KEY_LEFT, 0, KEYEVENTF_EXTENDEDKEY);
			if (backspaces == 1)
				backspaces--;
		} else {
			sendEmptyCharacter();
			backspaces++;
		}
	}

	sendBackspaces(backspacesToSend(backspaces));

	if (!vSendKeyStepByStep) {
		sendByClipboard(edit);
		return true;
	}
	for (uint32_t data : edit.chars) {
		sendKeyCode(data);
	}
	if (edit.retypeKey) {
		bool caps = (modifiers & MASK_CAPITAL) || (modifiers & MASK_SHIFT);
		sendKeyCode(keyCode | (caps ? CAPS_MASK : 0));
	}
	if (edit.startsNewSession) {
		startNewSession();
	}
	return true;
}

static LRESULT CALLBACK keyboardHookProcess(int nCode, WPARAM wParam, LPARAM lParam) {
	const KBDLLHOOKSTRUCT* input = (const KBDLLHOOKSTRUCT*)lParam;
	// Passing the key on must happen only when we do not consume it.
	auto passOn = [&]() { return CallNextHookEx(keyboardHook, nCode, wParam, lParam); };
	if (input->dwExtraInfo != 0 || isImeOpen()) { // our own events, or an IME is in charge
		return passOn();
	}

	bool isKeyDown = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
	bool isKeyUp = wParam == WM_KEYUP || wParam == WM_SYSKEYUP;
	uint32_t bit = modifierBit((uint16_t)input->vkCode);
	if (isKeyDown) {
		// The caps lock state can change without a key press we see, so read the real state before each key.
		modifiers = GetKeyState(VK_CAPITAL) == 1 ? modifiers | MASK_CAPITAL : modifiers & ~MASK_CAPITAL;
		modifiers |= bit;
		isModifierKey = bit != 0;
	} else if (isKeyUp) {
		modifiers &= ~bit;
		isModifierKey = bit != 0;
	}
	if (!isModifierKey && !isKeyUp) {
		keyCode = (uint16_t)input->vkCode;
	}

	if (isKeyDown && !isModifierKey && keyCode != 0) {
		if (GET_SWITCH_KEY(vSwitchKeyStatus) != keyCode) {
			lastModifiers = 0;
		} else if (matchesSwitchKey()) {
			switchLanguage();
			justUsedHotKey = true;
			keyCode = 0;
			return -1;
		}
		justUsedHotKey = lastModifiers != 0;
	} else if (isModifierKey) {
		if (lastModifiers == 0 || lastModifiers < modifiers) {
			lastModifiers = modifiers;
		} else if (lastModifiers > modifiers) {
			if (matchesSwitchKey()) {
				switchLanguage();
				justUsedHotKey = true;
			}
			if (vTempOffSpelling && !justUsedHotKey && (lastModifiers & MASK_CONTROL)) {
				vTempOffSpellChecking();
			}
			lastModifiers = modifiers;
			justUsedHotKey = false;
		}
		keyCode = 0;
		return passOn();
	}

	if (vLanguage == LANGUAGE_ENGLISH) {
		return handleEnglishKey(isKeyDown) ? -1 : passOn();
	}
	if (!isKeyDown) {
		return passOn();
	}

	vKeyHandleEvent(vKeyEvent::Keyboard, vKeyEventState::KeyDown, keyCode, capsStatus(), hasControlModifier());
	return applyEngineResult() ? -1 : passOn();
}

/*---------------------- mouse and focus hooks ----------------------*/

static LRESULT CALLBACK mouseHookProcess(int nCode, WPARAM wParam, LPARAM lParam) {
	switch (wParam) {
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
		case WM_NCXBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		case WM_NCXBUTTONUP:
			vKeyHandleEvent(vKeyEvent::Mouse, vKeyEventState::MouseDown, 0);
			break;
	}
	return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

static VOID CALLBACK foregroundEventProcess(HWINEVENTHOOK, DWORD, HWND, LONG, LONG, DWORD, DWORD) {
	vTempOffEngine(false);
	startNewSession();
	applyAppRule();
}
