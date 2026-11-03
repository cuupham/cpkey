#pragma once
#include "common.h"

#define RULE_NONE -1 // no rule for the app; any other value is a Language

void loadAppRules();
int getAppRule(const string& exe);
void setAppRule(const string& exe, int mode);
void removeAppRule(const string& exe);
const map<string, int>& getAllAppRules();
LPCTSTR ruleModeText(int mode);
