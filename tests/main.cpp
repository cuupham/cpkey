#include "Suites.h"
#include "Test.h"

int main() {
    runEngineTests();
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
