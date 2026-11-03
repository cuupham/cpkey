#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include <shellapi.h>
#include <commctrl.h>
#include <psapi.h>

#include "resource.h"

#include "../engine/Engine.h"

#include "KeyHook.h"
#include "Settings.h"
#include "SystemHelper.h"
#include "SystemTrayHelper.h"

using namespace std;

#define APP_NAME  _T(APP_NAME_TEXT)
#define APP_CLASS APP_NAME _T("VietnameseInputMethod")

#define APP_SET_DATA(KEY, VAL) \
	KEY = VAL;                 \
	SystemHelper::setIntSetting(_T(#KEY), KEY)
#define APP_GET_DATA(KEY, DEFAULT_VAL) KEY = SystemHelper::getIntSetting(_T(#KEY), DEFAULT_VAL)
