#include "QuickTyping.h"

#define INI_QUICK_TYPING _T("QuickTyping")

struct QuickRule {
	wstring trigger;
	wstring output;
	vector<Uint32> keys;  // what the engine records while the trigger is typed
	vector<Uint32> codes; // characters typed instead
};

static vector<QuickRule> _rules;

static bool isAsciiLetter(wchar_t c) {
	return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z');
}

static bool containsAny(const wstring& text, const wchar_t* letters) {
	return text.find_first_of(letters) != wstring::npos;
}

// Telex turns these sequences into accented letters or tones, so the typed keys would no longer
// match what is on screen and the wrong number of characters would be erased.
static bool formsVietnameseLetter(const wstring& trigger) {
	wstring lower = trigger;
	transform(lower.begin(), lower.end(), lower.begin(), [](wchar_t c) { return (wchar_t)towlower(c); });
	if (containsAny(lower, L"w") || lower.find(L"aa") != wstring::npos || lower.find(L"ee") != wstring::npos ||
		lower.find(L"oo") != wstring::npos || lower.find(L"dd") != wstring::npos) {
		return true;
	}
	for (size_t i = 1; i < lower.size(); i++) {
		if (wcschr(L"aeiouy", lower[i - 1]) && wcschr(L"sfrxjz", lower[i])) {
			return true;
		}
	}
	return false;
}

static void saveQuickTyping() {
	vector<pair<wstring, wstring>> entries;
	for (size_t i = 0; i < _rules.size(); i++) {
		// A trigger has no spaces, so the first space separates it from the output.
		entries.emplace_back(to_wstring(i + 1), _rules[i].trigger + L" " + _rules[i].output);
	}
	SystemHelper::writeSection(INI_QUICK_TYPING, entries);
}

wstring checkQuickTrigger(const wstring& trigger) {
	if (trigger.empty()) return L"Nhập chuỗi cần gõ.";
	if (trigger.size() > QUICK_TYPING_MAX_TRIGGER) return L"Tối đa " + to_wstring(QUICK_TYPING_MAX_TRIGGER) + L" ký tự.";
	if (!all_of(trigger.begin(), trigger.end(), isAsciiLetter)) {
		return L"Chỉ dùng chữ cái a-z, không dấu, không khoảng trắng, không số và dấu câu.";
	}
	if (formsVietnameseLetter(trigger)) {
		return L"Chuỗi này tạo thành chữ có dấu khi gõ Telex (ví dụ aa, dd, as, w). Hãy đổi chuỗi khác.";
	}
	return L"";
}

wstring checkQuickOutput(const wstring& output) {
	if (output.empty()) return L"Nhập nội dung thay thế.";
	if (output.size() > QUICK_TYPING_MAX_OUTPUT) return L"Tối đa " + to_wstring(QUICK_TYPING_MAX_OUTPUT) + L" ký tự.";
	if (any_of(output.begin(), output.end(), [](wchar_t c) { return c < L' '; })) return L"Nội dung chỉ gồm một dòng.";
	return L"";
}

static QuickRule makeRule(const wstring& trigger, const wstring& output) {
	QuickRule rule = { trigger, output };
	for (wchar_t c : trigger) {
		rule.keys.push_back(_characterMap[c]);
	}
	for (wchar_t c : output) {
		rule.codes.push_back((Uint32)c | PURE_CHARACTER_MASK);
	}
	return rule;
}

static vector<QuickRule>::iterator findRule(const wstring& trigger) {
	return find_if(_rules.begin(), _rules.end(), [&](const QuickRule& rule) { return rule.trigger == trigger; });
}

void loadQuickTyping() {
	_rules.clear();
	for (const auto& entry : SystemHelper::readSection(INI_QUICK_TYPING)) {
		size_t space = entry.second.find(L' ');
		if (space == wstring::npos) continue;
		wstring trigger = entry.second.substr(0, space), output = entry.second.substr(space + 1);
		if (checkQuickTrigger(trigger).empty() && checkQuickOutput(output).empty() && findRule(trigger) == _rules.end()) {
			_rules.push_back(makeRule(trigger, output));
		}
	}
}

vector<pair<wstring, wstring>> getQuickTyping() {
	vector<pair<wstring, wstring>> rules;
	for (const QuickRule& rule : _rules) {
		rules.emplace_back(rule.trigger, rule.output);
	}
	return rules;
}

bool hasQuickTyping(const wstring& trigger) {
	return findRule(trigger) != _rules.end();
}

void setQuickTyping(const wstring& trigger, const wstring& output) {
	auto existing = findRule(trigger);
	if (existing != _rules.end()) {
		*existing = makeRule(trigger, output);
	} else {
		_rules.push_back(makeRule(trigger, output));
	}
	saveQuickTyping();
}

void removeQuickTyping(const wstring& trigger) {
	auto existing = findRule(trigger);
	if (existing != _rules.end()) {
		_rules.erase(existing);
		saveQuickTyping();
	}
}

// Called by the engine at every word break.
bool findMacro(vector<Uint32>& key, vector<Uint32>& macroContentCode) {
	for (const QuickRule& rule : _rules) {
		if (rule.keys == key) {
			macroContentCode = rule.codes;
			return true;
		}
	}
	return false;
}
