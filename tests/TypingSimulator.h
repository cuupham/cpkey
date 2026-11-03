#pragma once
#include <string>

// Engine settings a typing case runs under.
struct TypingProfile {
    const char* name;
    int inputType;
    int checkSpelling;
    int modernOrthography;
    int restoreIfWrongSpelling;
    int upperCaseFirstChar;
    int allowConsonantZFWJ;
    int useMacro;
    bool english; // typing English: only quick typing is active
};

const TypingProfile* findProfile(const std::string& name);

// Registers the quick typing rules the macro profiles rely on, and removes them again.
void setUpQuickTypingRules();
void tearDownQuickTypingRules();

// Types `keys` into an empty text field the way the application does and returns what ends up on
// screen as UTF-8. Shifted characters use Shift; "{BS}" is Backspace.
std::string typeKeys(const TypingProfile& profile, const std::string& keys);
