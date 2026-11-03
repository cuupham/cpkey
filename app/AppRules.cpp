#include "AppRules.h"

#define INI_APP_RULES _T("AppRules")

static map<string, int> appRules;

// Windows treats exe names case-insensitively.
static string normalize(string exe) {
	transform(exe.begin(), exe.end(), exe.begin(), [](unsigned char c) { return c < 128 ? tolower(c) : c; });
	return exe;
}

static void saveAppRules() {
	vector<pair<wstring, wstring>> entries;
	for (const auto& rule : appRules) {
		entries.emplace_back(Os::toWide(rule.first), to_wstring(rule.second));
	}
	Os::writeSection(INI_APP_RULES, entries);
}

void loadAppRules() {
	appRules.clear();
	for (const auto& entry : Os::readSection(INI_APP_RULES)) {
		appRules[Os::toUtf8(entry.first)] = _wtoi(entry.second.c_str());
	}
}

int getAppRule(const string& exe) {
	auto it = appRules.find(normalize(exe));
	return it == appRules.end() ? RULE_NONE : it->second;
}

void setAppRule(const string& exe, int mode) {
	appRules[normalize(exe)] = mode;
	saveAppRules();
	reapplyAppRule();
}

void removeAppRule(const string& exe) {
	appRules.erase(normalize(exe));
	saveAppRules();
	reapplyAppRule();
}

const map<string, int>& getAllAppRules() {
	return appRules;
}

LPCTSTR ruleModeText(int mode) {
	return mode == LANGUAGE_VIETNAMESE ? _T("Luôn tiếng Việt") : _T("Luôn tiếng Anh");
}
