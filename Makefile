# Build with MinGW-w64 (w64devkit): put its bin directory on PATH, then run `make`.
OUT := build
# The product name lives only in app/resource.h.
APP_NAME := $(shell sed -n 's/.*APP_NAME_TEXT *"\(.*\)"/\1/p' app/resource.h)
TARGET := $(OUT)/$(APP_NAME).exe

SRC := $(wildcard app/*.cpp engine/*.cpp)
OBJ := $(SRC:%.cpp=$(OUT)/%.o) $(OUT)/app/app-res.o

DEFINES := -DUNICODE -D_UNICODE -DNDEBUG -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00
# C++14: C++17's std::byte clashes with the Windows SDK's `byte` under `using namespace std`.
# No exceptions or RTTI are used, so dropping them shrinks the exe.
CXXFLAGS := -std=gnu++14 -Os $(DEFINES) -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -MMD -MP
# `{ 0 }` zero-initialises Win32 structs on purpose; the engine compares signed and unsigned sizes.
WARNINGS := -Wall -Wextra -Wshadow -Wno-missing-field-initializers -Wno-unused-parameter -Wno-sign-compare -Wno-conversion-null
LDFLAGS := -mwindows -municode -static -s -Wl,--gc-sections
LIBS := -lcomctl32 -lcomdlg32 -limm32 -lshell32 -ladvapi32 -lpsapi -lgdi32 -ldwmapi -luxtheme

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OUT)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(WARNINGS) -c $< -o $@

$(OUT)/app/app-res.o: app/app.rc app/resource.h app/app.manifest
	@mkdir -p $(dir $@)
	windres $(DEFINES) -Iapp -c 65001 $< -O coff -o $@

# Tests link the engine and the modules that need no window or hook. Run from the repo root.
TEST_SRC := $(wildcard tests/*.cpp)
TEST_OBJ := $(TEST_SRC:%.cpp=$(OUT)/%.o)
TEST_DEPS := $(addprefix $(OUT)/app/,Os.o Settings.o QuickTyping.o AppRules.o) $(filter $(OUT)/engine/%,$(OBJ))
TEST_TARGET := $(OUT)/tests/$(APP_NAME)-test.exe

$(TEST_TARGET): $(TEST_OBJ) $(TEST_DEPS)
	$(CXX) -static -o $@ $^ $(LIBS)

test: $(TEST_TARGET)
	$(TEST_TARGET)

test-update-snapshot: $(TEST_TARGET)
	$(TEST_TARGET) --update-snapshot

# Needs clang-format on PATH. resource.h belongs to the resource editor, so it is left alone.
FORMAT_FILES := $(SRC) $(TEST_SRC) $(filter-out app/resource.h,$(wildcard app/*.h engine/*.h engine/platforms/*.h tests/*.h))

format:
	clang-format -i $(FORMAT_FILES)

format-check:
	clang-format --dry-run --Werror $(FORMAT_FILES)

clean:
	rm -rf $(OUT)

-include $(OBJ:.o=.d) $(TEST_OBJ:.o=.d)
.PHONY: all clean test test-update-snapshot format format-check
