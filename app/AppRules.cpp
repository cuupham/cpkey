#include "AppRules.h"

#define INI_APP_RULES _T("AppRules")

static map<string, int> _rules;

// Windows treats exe names case-insensitively.
static string normalize(string exe) {
	transform(exe.begin(), exe.end(), exe.begin(), [](unsigned char c) { return c < 128 ? tolower(c) : c; });
	return exe;
}

static void saveAppRules() {
	vector<pair<wstring, wstring>> entries;
	for (const auto& rule : _rules) {
		entries.emplace_back(SystemHelper::toWide(rule.first), to_wstring(rule.second));
	}
	SystemHelper::writeSection(INI_APP_RULES, entries);
}

void loadAppRules() {
	_rules.clear();
	for (const auto& entry : SystemHelper::readSection(INI_APP_RULES)) {
		_rules[SystemHelper::toUtf8(entry.first)] = _wtoi(entry.second.c_str());
	}
}

int getAppRule(const string& exe) {
	auto it = _rules.find(normalize(exe));
	return it == _rules.end() ? RULE_NONE : it->second;
}

void setAppRule(const string& exe, int mode) {
	_rules[normalize(exe)] = mode;
	saveAppRules();
	reapplyAppRule();
}

void removeAppRule(const string& exe) {
	_rules.erase(normalize(exe));
	saveAppRules();
	reapplyAppRule();
}

const map<string, int>& getAllAppRules() {
	return _rules;
}

LPCTSTR ruleModeText(int mode) {
	return mode == RULE_VIETNAMESE ? _T("Luôn tiếng Việt") : _T("Luôn tiếng Anh");
}
