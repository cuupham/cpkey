#pragma once
#include <cstdio>
#include <string>

inline int& failureCount() {
    static int count = 0;
    return count;
}

inline void reportFailure(const char* file, int line, const std::string& message) {
    std::printf("FAIL %s:%d %s\n", file, line, message.c_str());
    failureCount()++;
}

inline std::string describe(const std::string& value) {
    return "\"" + value + "\"";
}

inline std::string describe(int value) {
    return std::to_string(value);
}

template <class Actual, class Expected>
void checkEqual(const char* file, int line, const char* expression, const Actual& actual, const Expected& expected) {
    if (!(actual == expected)) {
        reportFailure(file, line, std::string(expression) + ": expected " + describe(expected) + ", got " + describe(actual));
    }
}

#define CHECK(condition)                                                 \
    do {                                                                 \
        if (!(condition)) reportFailure(__FILE__, __LINE__, #condition); \
    } while (0)

#define CHECK_EQ(actual, expected) checkEqual(__FILE__, __LINE__, #actual, (actual), (expected))
