#pragma once

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
#include "Os.h"
#include "Tray.h"

using namespace std;

#define APP_NAME  _T(APP_NAME_TEXT)
#define APP_CLASS APP_NAME _T("VietnameseInputMethod")

// Sent to the running instance to open its control panel.
#define WM_SHOW_CONTROL_PANEL (WM_USER + 2019)

#define APP_SET_DATA(KEY, VAL) \
	KEY = VAL;                 \
	Os::setIntSetting(_T(#KEY), KEY)
#define APP_GET_DATA(KEY, DEFAULT_VAL) KEY = Os::getIntSetting(_T(#KEY), DEFAULT_VAL)
