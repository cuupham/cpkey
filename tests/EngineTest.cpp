#include "Suites.h"
#include "Test.h"
#include "TypingSimulator.h"
#include <fstream>
#include <vector>

// Each line is `profile<TAB>keys<TAB>expected text`. Run from the repository root.
// engine-cases.tsv holds hand-checked Vietnamese. engine-snapshot.tsv records what the engine produced
// for a wide spread of inputs, so a refactor that changes any typing result shows up here. After an
// intended engine change, review the result and rewrite the snapshot with `make test-update-snapshot`.
#define CASES_FILE    "tests/engine-cases.tsv"
#define SNAPSHOT_FILE "tests/engine-snapshot.tsv"

struct Case {
    const TypingProfile* profile;
    std::string keys;
    std::string expected;
};

static bool isCaseLine(const std::string& line) {
    return !line.empty() && line[0] != '#';
}

// Returns false for a line that does not hold a known profile and three columns.
static bool parseCase(const std::string& line, Case& result) {
    size_t firstTab = line.find('\t'), secondTab = line.find('\t', firstTab + 1);
    if (secondTab == std::string::npos) return false;
    result.profile = findProfile(line.substr(0, firstTab));
    result.keys = line.substr(firstTab + 1, secondTab - firstTab - 1);
    result.expected = line.substr(secondTab + 1);
    return result.profile != nullptr;
}

static void runCaseFile(const char* path) {
    std::ifstream file(path);
    CHECK(file.is_open());
    std::string line;
    for (int lineNumber = 1; std::getline(file, line); lineNumber++) {
        if (!isCaseLine(line)) continue;
        Case current;
        if (!parseCase(line, current)) {
            reportFailure(path, lineNumber, "malformed case");
            continue;
        }
        std::string actual = typeKeys(*current.profile, current.keys);
        if (actual != current.expected) {
            reportFailure(path, lineNumber, std::string(current.profile->name) + " " + describe(current.keys) + ": expected " + describe(current.expected) + ", got " + describe(actual));
        }
    }
}

static void rewriteSnapshot(const char* path) {
    std::vector<std::string> lines;
    int changed = 0;
    {
        std::ifstream file(path);
        CHECK(file.is_open());
        std::string line;
        while (std::getline(file, line)) {
            Case current;
            if (isCaseLine(line) && parseCase(line, current)) {
                std::string actual = typeKeys(*current.profile, current.keys);
                if (actual != current.expected) changed++;
                line = std::string(current.profile->name) + "\t" + current.keys + "\t" + actual;
            }
            lines.push_back(line);
        }
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    for (const std::string& line : lines) {
        out << line << '\n';
    }
    std::printf("snapshot rewritten: %d case(s) changed\n", changed);
}

void runEngineTests(bool updateSnapshot) {
    setUpQuickTypingRules();
    if (updateSnapshot) {
        rewriteSnapshot(SNAPSHOT_FILE);
    }
    runCaseFile(CASES_FILE);
    runCaseFile(SNAPSHOT_FILE);
    tearDownQuickTypingRules();
}
