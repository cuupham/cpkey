#pragma once

// Global keyboard hook and the typing language it applies.

// Last key seen by the message loop; the control panel uses it to capture the language switch key.
extern unsigned short lastKeyCode;

// The language the hook is typing in right now (1 Vietnamese, 0 English). An app rule can override it.
extern int vLanguage;

void keyHookInit();
void keyHookFree();

// Applies the rule of the focused app, or restores the base language when it has none.
void applyAppRule();

// Same, but also for the app already in focus (call after the rules changed).
void reapplyAppRule();

// The base language is the user's own choice; it applies in every app without a rule.
int getBaseLanguage();
void setBaseLanguage(int language);

// Flips the focused app's language. Refused (returns false) while an app rule owns it.
bool toggleLanguage();
bool isLanguageLocked();
