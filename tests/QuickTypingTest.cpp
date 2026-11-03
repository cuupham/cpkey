#include "../app/QuickTyping.h"
#include "../app/common.h"
#include "Suites.h"
#include "Test.h"
#include <cstdint>

// Same section the application reads; written directly to feed the loader bad lines.
#define QUICK_TYPING_SECTION _T("QuickTyping")

static void clearRules() {
    for (const auto& rule : getQuickTyping()) {
        removeQuickTyping(rule.first);
    }
}

static void testTriggerValidation() {
    CHECK(checkQuickTrigger(L"vn").empty());
    CHECK(checkQuickTrigger(L"tphcm").empty());
    CHECK(checkQuickTrigger(L"Vn").empty());
    CHECK(checkQuickTrigger(L"ad").empty());
    CHECK(checkQuickTrigger(wstring(QUICK_TYPING_MAX_TRIGGER, L'b')).empty());

    CHECK(!checkQuickTrigger(L"").empty());
    CHECK(!checkQuickTrigger(wstring(QUICK_TYPING_MAX_TRIGGER + 1, L'b')).empty());
    CHECK(!checkQuickTrigger(L"v1").empty());
    CHECK(!checkQuickTrigger(L"v n").empty());
    CHECK(!checkQuickTrigger(L"vé").empty());
    CHECK(!checkQuickTrigger(L"v.").empty());
}

// Telex would turn these into accented letters, so the typed keys would not match the screen.
static void testTriggerRejectsTelexSequences() {
    for (const wchar_t* trigger : { L"aa", L"ee", L"oo", L"dd", L"AA", L"w", L"vnw", L"as", L"af", L"ar", L"ax", L"aj", L"az", L"xyz" }) {
        CHECK(!checkQuickTrigger(trigger).empty());
    }
}

static void testOutputValidation() {
    CHECK(checkQuickOutput(L"Việt Nam").empty());
    CHECK(checkQuickOutput(wstring(QUICK_TYPING_MAX_OUTPUT, L'x')).empty());

    CHECK(!checkQuickOutput(L"").empty());
    CHECK(!checkQuickOutput(wstring(QUICK_TYPING_MAX_OUTPUT + 1, L'x')).empty());
    CHECK(!checkQuickOutput(L"two\nlines").empty());
    CHECK(!checkQuickOutput(L"tab\there").empty());
}

static void testStoreKeepsInsertionOrder() {
    clearRules();
    setQuickTyping(L"bb", L"first");
    setQuickTyping(L"cc", L"second");
    setQuickTyping(L"bb", L"changed");

    auto rules = getQuickTyping();
    CHECK_EQ(rules.size(), 2);
    CHECK(rules[0] == make_pair(wstring(L"bb"), wstring(L"changed")));
    CHECK(rules[1] == make_pair(wstring(L"cc"), wstring(L"second")));
    CHECK(hasQuickTyping(L"bb"));
    CHECK(!hasQuickTyping(L"dd"));

    removeQuickTyping(L"bb");
    removeQuickTyping(L"missing");
    CHECK(!hasQuickTyping(L"bb"));
    CHECK_EQ(getQuickTyping().size(), 1);
    clearRules();
}

static void testRulesSurviveReload() {
    clearRules();
    setQuickTyping(L"vn", L"Việt Nam");
    setQuickTyping(L"hn", L"Hà Nội");
    auto saved = getQuickTyping();

    loadQuickTyping();
    CHECK(getQuickTyping() == saved);
    clearRules();
}

static void testLoadSkipsInvalidLines() {
    clearRules();
    Os::writeSection(QUICK_TYPING_SECTION, { { L"1", L"aa telex" }, { L"2", L"vn Việt" }, { L"3", L"nospace" }, { L"4", L"vn duplicate" }, { L"5", L"ok fine" } });

    loadQuickTyping();
    auto rules = getQuickTyping();
    CHECK_EQ(rules.size(), 2);
    CHECK(rules[0] == make_pair(wstring(L"vn"), wstring(L"Việt")));
    CHECK(rules[1] == make_pair(wstring(L"ok"), wstring(L"fine")));
    clearRules();
}

static void testFindMacroMatchesExactKeys() {
    clearRules();
    setQuickTyping(L"vn", L"Việt");

    vector<uint32_t> output, typed = { KEY_V, KEY_N };
    CHECK(findMacro(typed, output));
    CHECK_EQ(output.size(), 4);
    CHECK_EQ(output[0], (uint32_t)L'V' | PURE_CHARACTER_MASK);

    vector<uint32_t> shifted = { KEY_V | CAPS_MASK, KEY_N };
    CHECK(!findMacro(shifted, output));
    vector<uint32_t> longer = { KEY_V, KEY_N, KEY_A };
    CHECK(!findMacro(longer, output));
    clearRules();
}

void runQuickTypingTests() {
    testTriggerValidation();
    testTriggerRejectsTelexSequences();
    testOutputValidation();
    testStoreKeepsInsertionOrder();
    testRulesSurviveReload();
    testLoadSkipsInvalidLines();
    testFindMacroMatchesExactKeys();
}
