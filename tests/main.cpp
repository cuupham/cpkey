#include "Suites.h"
#include "Test.h"

#define UPDATE_SNAPSHOT_FLAG "--update-snapshot"

int main(int argc, char** argv) {
    runEngineTests(argc > 1 && std::string(argv[1]) == UPDATE_SNAPSHOT_FLAG);
    runTextEditTests();
    runQuickTypingTests();
    runAppRulesTests();
    if (failureCount() > 0) {
        std::printf("%d check(s) failed\n", failureCount());
        return 1;
    }
    std::printf("all checks passed\n");
    return 0;
}
