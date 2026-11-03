#pragma once
#include "stdafx.h"

enum AppRuleMode {
	RULE_NONE = -1,
	RULE_ENGLISH = 0,
	RULE_VIETNAMESE = 1,
};

void loadAppRules();
int getAppRule(const string& exe);
void setAppRule(const string& exe, int mode);
void removeAppRule(const string& exe);
const map<string, int>& getAllAppRules();
LPCTSTR ruleModeText(int mode);
