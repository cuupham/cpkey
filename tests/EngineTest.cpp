#include "Suites.h"
#include "Test.h"
#include "TypingSimulator.h"
#include <fstream>

// Each line is `profile<TAB>keys<TAB>expected text`. Run from the repository root.
// engine-cases.tsv holds hand-checked Vietnamese. engine-snapshot.tsv records what the engine produced
// for a wide spread of inputs, so a refactor that changes any typing result shows up here.
static const char* CASE_FILES[] = { "tests/engine-cases.tsv", "tests/engine-snapshot.tsv" };

static void runCaseFile(const char* path) {
    std::ifstream file(path);
    CHECK(file.is_open());
    std::string line;
    for (int lineNumber = 1; std::getline(file, line); lineNumber++) {
        if (line.empty() || line[0] == '#') continue;
        size_t firstTab = line.find('\t'), secondTab = line.find('\t', firstTab + 1);
        if (secondTab == std::string::npos) {
            reportFailure(path, lineNumber, "malformed case");
            continue;
        }
        const TypingProfile* profile = findProfile(line.substr(0, firstTab));
        if (!profile) {
            reportFailure(path, lineNumber, "unknown profile");
            continue;
        }
        std::string keys = line.substr(firstTab + 1, secondTab - firstTab - 1);
        std::string expected = line.substr(secondTab + 1);
        std::string actual = typeKeys(*profile, keys);
        if (actual != expected) {
            reportFailure(path, lineNumber, std::string(profile->name) + " " + describe(keys) + ": expected " + describe(expected) + ", got " + describe(actual));
        }
    }
}

void runEngineTests() {
    setUpQuickTypingRules();
    for (const char* path : CASE_FILES) {
        runCaseFile(path);
    }
    tearDownQuickTypingRules();
}
