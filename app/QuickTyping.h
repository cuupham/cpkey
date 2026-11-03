#pragma once
#include "stdafx.h"

// User-defined abbreviations: typing a word that matches a trigger exactly (case-sensitive)
// and then a word break replaces it with the output.

#define QUICK_TYPING_MAX_TRIGGER 20
#define QUICK_TYPING_MAX_OUTPUT 200

void loadQuickTyping();

// (trigger, output) pairs in the order they were added.
vector<pair<wstring, wstring>> getQuickTyping();
bool hasQuickTyping(const wstring& trigger);

// Each check returns an error text, or an empty string when the value is acceptable.
wstring checkQuickTrigger(const wstring& trigger);
wstring checkQuickOutput(const wstring& output);

// Adds the rule, or replaces the output of an existing trigger. Inputs must pass the checks.
void setQuickTyping(const wstring& trigger, const wstring& output);
void removeQuickTyping(const wstring& trigger);
