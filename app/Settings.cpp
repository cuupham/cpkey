#include "Settings.h"

#define DEFINE_SETTING(name, value) int name = value;
PERSISTED_SETTINGS(DEFINE_SETTING)
