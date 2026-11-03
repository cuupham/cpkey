#include "TypingSimulator.h"
#include "../app/QuickTyping.h"
#include "../engine/Engine.h"
#include <cstdint>
#include <cstring>

static const TypingProfile PROFILES[] = {
    //  name               input          spell modern restore upper zfwj macro english
    { "telex", vTelex, 1, 0, 1, 0, 0, 0, false },
    { "telex-modern", vTelex, 1, 1, 1, 0, 0, 0, false },
    { "telex-nospell", vTelex, 0, 0, 0, 0, 0, 0, false },
    { "telex-norestore", vTelex, 1, 0, 0, 0, 0, 0, false },
    { "telex-zfwj", vTelex, 1, 0, 1, 0, 1, 0, false },
    { "telex-capsfirst", vTelex, 1, 0, 1, 1, 0, 0, false },
    { "telex-macro", vTelex, 1, 0, 1, 0, 0, 1, false },
    { "english-macro", vTelex, 1, 0, 1, 0, 0, 1, true },
    { "vni", vVNI, 1, 0, 1, 0, 0, 0, false },
    { "vni-modern", vVNI, 1, 1, 1, 0, 0, 0, false },
    { "vni-nospell", vVNI, 0, 0, 0, 0, 0, 0, false },
    { "simple-telex", vSimpleTelex1, 1, 0, 1, 0, 0, 0, false },
    { "simple-telex-2", vSimpleTelex2, 1, 0, 1, 0, 0, 0, false },
};

#define BACKSPACE_TOKEN "{BS}"

static const vKeyHookState* engineState;

const TypingProfile* findProfile(const std::string& name) {
    for (const TypingProfile& profile : PROFILES) {
        if (name == profile.name) return &profile;
    }
    return nullptr;
}

void setUpQuickTypingRules() {
    setQuickTyping(L"vn", L"Việt Nam");
    setQuickTyping(L"tphcm", L"Thành phố Hồ Chí Minh");
}

void tearDownQuickTypingRules() {
    removeQuickTyping(L"vn");
    removeQuickTyping(L"tphcm");
}

static void applySettings(const TypingProfile& profile) {
    vInputType = profile.inputType;
    vCheckSpelling = profile.checkSpelling;
    vUseModernOrthography = profile.modernOrthography;
    vRestoreIfWrongSpelling = profile.restoreIfWrongSpelling;
    vUpperCaseFirstChar = profile.upperCaseFirstChar;
    vAllowConsonantZFWJ = profile.allowConsonantZFWJ;
    vUseMacro = profile.useMacro;
}

static std::string toUtf8(const std::u16string& text) {
    std::string out;
    for (char16_t c : text) {
        if (c < 0x80) {
            out += (char)c;
        } else if (c < 0x800) {
            out += (char)(0xC0 | (c >> 6));
            out += (char)(0x80 | (c & 0x3F));
        } else {
            out += (char)(0xE0 | (c >> 12));
            out += (char)(0x80 | ((c >> 6) & 0x3F));
            out += (char)(0x80 | (c & 0x3F));
        }
    }
    return out;
}

// What the application sends to the focused window for one engine character.
static void appendEngineChar(std::u16string& screen, uint32_t data) {
    if (data & CHAR_CODE_MASK) {
        screen += (char16_t)data;
    } else if (uint16_t character = keyCodeToCharacter(data)) {
        screen += (char16_t)character;
    }
}

static void deleteLast(std::u16string& screen, int count) {
    for (int i = 0; i < count && !screen.empty(); i++) {
        screen.pop_back();
    }
}

// Mirrors the engine-result handling of app/KeyHook.cpp, minus the browser workarounds.
static void applyEngineResult(std::u16string& screen, uint16_t keyCode, bool shift) {
    const vKeyHookState* state = engineState;
    uint32_t keyWithCaps = keyCode | (shift ? CAPS_MASK : 0);
    uint8_t code = state->code;
    if (code == vReplaceMacro) {
        deleteLast(screen, state->backspaceCount);
        for (uint32_t macroChar : state->macroData) {
            screen += (char16_t)macroChar;
        }
        appendEngineChar(screen, keyWithCaps);
        return;
    }
    if (code != vWillProcess && code != vRestore && code != vRestoreAndStartNewSession) {
        if (keyCode == KEY_DELETE) {
            deleteLast(screen, 1);
        } else {
            appendEngineChar(screen, keyWithCaps);
        }
        return;
    }
    if (state->backspaceCount > 0 && state->backspaceCount < MAX_BUFF) {
        deleteLast(screen, state->backspaceCount);
    }
    for (int i = state->newCharCount - 1; i >= 0; i--) {
        appendEngineChar(screen, state->charData[i]);
    }
    if (code == vRestore || code == vRestoreAndStartNewSession) {
        appendEngineChar(screen, keyWithCaps);
    }
    if (code == vRestoreAndStartNewSession) {
        startNewSession();
    }
}

std::string typeKeys(const TypingProfile& profile, const std::string& keys) {
    applySettings(profile);
    engineState = (const vKeyHookState*)vKeyInit();
    startNewSession();
    std::u16string screen;
    for (size_t i = 0; i < keys.size(); i++) {
        uint16_t keyCode;
        bool shift = false;
        if (keys.compare(i, strlen(BACKSPACE_TOKEN), BACKSPACE_TOKEN) == 0) {
            keyCode = KEY_DELETE;
            i += strlen(BACKSPACE_TOKEN) - 1;
        } else {
            uint32_t mapped = _characterMap[(unsigned char)keys[i]];
            keyCode = mapped & ~CAPS_MASK;
            shift = mapped & CAPS_MASK;
        }
        if (profile.english) {
            vEnglishMode(vKeyEventState::KeyDown, keyCode, shift, false);
            if (engineState->code == vReplaceMacro) {
                applyEngineResult(screen, keyCode, shift);
            } else if (keyCode == KEY_DELETE) {
                deleteLast(screen, 1);
            } else {
                appendEngineChar(screen, keyCode | (shift ? CAPS_MASK : 0));
            }
            continue;
        }
        vKeyHandleEvent(vKeyEvent::Keyboard, vKeyEventState::KeyDown, keyCode, shift ? 1 : 0, false);
        applyEngineResult(screen, keyCode, shift);
    }
    return toUtf8(screen);
}
