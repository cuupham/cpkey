#include "../app/AppRules.h"
#include "../app/common.h"
#include "Suites.h"
#include "Test.h"

// Same section the application reads; written directly to feed the loader.
#define APP_RULES_SECTION _T("AppRules")

static int reapplyCount = 0;

// AppRules asks the hook to re-read the focused app's rule; no hook runs in tests.
void reapplyAppRule() {
    reapplyCount++;
}

static void clearRules() {
    while (!getAllAppRules().empty()) {
        removeAppRule(getAllAppRules().begin()->first);
    }
}

static void testUnknownAppHasNoRule() {
    clearRules();
    CHECK_EQ(getAppRule("notepad.exe"), RULE_NONE);
}

static void testExeNamesIgnoreCase() {
    clearRules();
    setAppRule("Chrome.EXE", LANGUAGE_VIETNAMESE);

    CHECK_EQ(getAppRule("chrome.exe"), LANGUAGE_VIETNAMESE);
    CHECK_EQ(getAppRule("CHROME.exe"), LANGUAGE_VIETNAMESE);
    CHECK_EQ(getAllAppRules().size(), 1);
    CHECK(getAllAppRules().count("chrome.exe") == 1);

    setAppRule("chrome.exe", LANGUAGE_ENGLISH);
    CHECK_EQ(getAllAppRules().size(), 1);
    CHECK_EQ(getAppRule("Chrome.exe"), LANGUAGE_ENGLISH);
    clearRules();
}

static void testChangesReapplyTheFocusedAppRule() {
    clearRules();
    reapplyCount = 0;
    setAppRule("code.exe", LANGUAGE_ENGLISH);
    CHECK_EQ(reapplyCount, 1);
    removeAppRule("code.exe");
    CHECK_EQ(reapplyCount, 2);
}

static void testRemove() {
    clearRules();
    setAppRule("a.exe", LANGUAGE_ENGLISH);
    setAppRule("b.exe", LANGUAGE_VIETNAMESE);

    removeAppRule("A.exe");
    CHECK_EQ(getAppRule("a.exe"), RULE_NONE);
    CHECK_EQ(getAppRule("b.exe"), LANGUAGE_VIETNAMESE);
    removeAppRule("missing.exe");
    CHECK_EQ(getAllAppRules().size(), 1);
    clearRules();
}

static void testRulesSurviveReload() {
    clearRules();
    setAppRule("word.exe", LANGUAGE_VIETNAMESE);
    setAppRule("ứng-dụng.exe", LANGUAGE_ENGLISH);
    auto saved = getAllAppRules();

    loadAppRules();
    CHECK(getAllAppRules() == saved);
    CHECK_EQ(getAppRule("ứng-dụng.exe"), LANGUAGE_ENGLISH);
    clearRules();
}

static void testLoadReadsPersistedSection() {
    clearRules();
    Os::writeSection(APP_RULES_SECTION, { { L"slack.exe", L"1" }, { L"game.exe", L"0" } });

    loadAppRules();
    CHECK_EQ(getAppRule("slack.exe"), LANGUAGE_VIETNAMESE);
    CHECK_EQ(getAppRule("game.exe"), LANGUAGE_ENGLISH);
    clearRules();
}

static void testModeTextDiffersPerLanguage() {
    CHECK(wcscmp(ruleModeText(LANGUAGE_VIETNAMESE), ruleModeText(LANGUAGE_ENGLISH)) != 0);
}

void runAppRulesTests() {
    testUnknownAppHasNoRule();
    testExeNamesIgnoreCase();
    testChangesReapplyTheFocusedAppRule();
    testRemove();
    testRulesSurviveReload();
    testLoadReadsPersistedSection();
    testModeTextDiffersPerLanguage();
}
