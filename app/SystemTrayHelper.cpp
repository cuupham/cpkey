#include "SystemTrayHelper.h"
#include "AppDelegate.h"

#define WM_TRAYMESSAGE (WM_USER + 1)
#define WM_SHOW_CONTROL_PANEL (WM_USER + 2019)
#define TRAY_ICONUID 100

#define POPUP_VIET_ON_OFF 900
#define POPUP_SPELLING 901

#define POPUP_TELEX 910
#define POPUP_VNI 911
#define POPUP_SIMPLE_TELEX 912

#define POPUP_APP_RULES 990
#define POPUP_CONTROL_PANEL 1000
#define POPUP_EXIT 2000

#define MODIFY_MENU(MENU, COMMAND, DATA) ModifyMenu(MENU, COMMAND, \
											MF_BYCOMMAND | (DATA ? MF_CHECKED : MF_UNCHECKED), \
											COMMAND, \
											menuData[COMMAND]);

static HMENU popupMenu;
static NOTIFYICONDATA nid;

static map<UINT, LPCTSTR> menuData = {
	{POPUP_VIET_ON_OFF, _T("Bật Tiếng Việt")},
	{POPUP_SPELLING, _T("Bật kiểm tra chính tả")},
	{POPUP_TELEX, _T("Kiểu gõ Telex")},
	{POPUP_VNI, _T("Kiểu gõ VNI")},
	{POPUP_SIMPLE_TELEX, _T("Kiểu gõ Simple Telex")},
	{POPUP_APP_RULES, _T("Cấu hình theo ứng dụng...")},
	{POPUP_CONTROL_PANEL, _T("Bảng điều khiển...")},
	{POPUP_EXIT, _T("Thoát")},
};

static void onTrayMenuCommand(UINT commandId) {
	AppDelegate* app = AppDelegate::getInstance();
	switch (commandId) {
	case POPUP_VIET_ON_OFF:
		app->onToggleVietnamese();
		break;
	case POPUP_SPELLING:
		app->onToggleCheckSpelling();
		break;
	case POPUP_TELEX:
		app->onInputType(0);
		break;
	case POPUP_VNI:
		app->onInputType(1);
		break;
	case POPUP_SIMPLE_TELEX:
		app->onInputType(2);
		break;
	case POPUP_APP_RULES:
		app->onAppRules();
		break;
	case POPUP_CONTROL_PANEL:
		app->onControlPanel();
		break;
	case POPUP_EXIT:
		app->onExit();
		break;
	}
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static UINT taskbarCreated;

	switch (message) {
	case WM_CREATE:
		taskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
		break;
	case WM_SHOW_CONTROL_PANEL:
		AppDelegate::getInstance()->onControlPanel();
		break;
	case WM_TRAYMESSAGE:
		if (lParam == WM_LBUTTONDBLCLK) {
			AppDelegate::getInstance()->onControlPanel();
		} else if (lParam == WM_LBUTTONUP) {
			AppDelegate::getInstance()->onToggleVietnamese();
		} else if (lParam == WM_RBUTTONDOWN) {
			POINT curPoint;
			GetCursorPos(&curPoint);
			SetForegroundWindow(hWnd);
			UINT commandId = TrackPopupMenu(popupMenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hWnd, NULL);
			onTrayMenuCommand(commandId);
			SystemTrayHelper::updateData();
		}
		break;
	default:
		// if the taskbar is restarted, add the system tray icon again
		if (message == taskbarCreated) {
			Shell_NotifyIcon(NIM_ADD, &nid);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

HWND SystemTrayHelper::createFakeWindow(const HINSTANCE & hIns) {
	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hIns;
	wcex.hIcon = LoadIcon(hIns, MAKEINTRESOURCE(IDI_APP_ICON));
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = APP_CLASS;
	RegisterClassExW(&wcex);
	HWND hWnd = CreateWindowW(APP_CLASS, _T(""), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hIns, nullptr);
	if (!hWnd) {
		return NULL;
	}
	ShowWindow(hWnd, 0);
	UpdateWindow(hWnd);
	return hWnd;
}

void SystemTrayHelper::createPopupMenu() {
	popupMenu = CreatePopupMenu();
	AppendMenu(popupMenu, MF_CHECKED, POPUP_VIET_ON_OFF, menuData[POPUP_VIET_ON_OFF]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_SPELLING, menuData[POPUP_SPELLING]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_TELEX, menuData[POPUP_TELEX]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_VNI, menuData[POPUP_VNI]);
	AppendMenu(popupMenu, MF_CHECKED, POPUP_SIMPLE_TELEX, menuData[POPUP_SIMPLE_TELEX]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(popupMenu, MF_STRING, POPUP_APP_RULES, menuData[POPUP_APP_RULES]);
	AppendMenu(popupMenu, MF_STRING, POPUP_CONTROL_PANEL, menuData[POPUP_CONTROL_PANEL]);
	AppendMenu(popupMenu, MF_SEPARATOR, 0, 0);
	AppendMenu(popupMenu, MF_UNCHECKED, POPUP_EXIT, menuData[POPUP_EXIT]);

	SetMenuDefaultItem(popupMenu, POPUP_CONTROL_PANEL, false);
}

static void loadTrayIcon() {
	int icon;
	if (vLanguage) {
		icon = vUseGrayIcon ? IDI_ICON_STATUS_VIET_10 : IDI_ICON_STATUS_VIET;
		LoadString(GetModuleHandle(0), IDS_TRAY_TITLE_2, nid.szTip, 128);
	} else {
		icon = vUseGrayIcon ? IDI_ICON_STATUS_ENG_10 : IDI_ICON_STATUS_ENG;
		LoadString(GetModuleHandle(0), IDS_TRAY_TITLE, nid.szTip, 128);
	}
	nid.hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(icon));
}

void SystemTrayHelper::updateData() {
	loadTrayIcon();
	Shell_NotifyIcon(NIM_MODIFY, &nid);

	MODIFY_MENU(popupMenu, POPUP_VIET_ON_OFF, vLanguage);
	EnableMenuItem(popupMenu, POPUP_VIET_ON_OFF, MF_BYCOMMAND | (isLanguageLocked() ? MF_GRAYED : MF_ENABLED));
	MODIFY_MENU(popupMenu, POPUP_SPELLING, vCheckSpelling);
	MODIFY_MENU(popupMenu, POPUP_TELEX, vInputType == 0);
	MODIFY_MENU(popupMenu, POPUP_VNI, vInputType == 1);
	MODIFY_MENU(popupMenu, POPUP_SIMPLE_TELEX, vInputType == 2);
}

static HINSTANCE ins;
static int recreateCount = 0;

void SystemTrayHelper::_createSystemTrayIcon(const HINSTANCE& hIns) {
	ins = hIns;
	HWND hWnd = createFakeWindow(ins);

	if (hWnd == NULL) { //Use timer to create
		if (recreateCount >= 5) {
			PostQuitMessage(0);
			return;
		}
		SetTimer(NULL, 0, 1000 * 3, WaitToCreateFakeWindow);
		++recreateCount;
		return;
	}
	createPopupMenu();

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = TRAY_ICONUID;
	nid.uVersion = NOTIFYICON_VERSION;
	nid.uCallbackMessage = WM_TRAYMESSAGE;
	loadTrayIcon();
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;

	// Shell_NotifyIcon may fail if the system tray icon is not fully initialized
	const int maxRetries = 5;
	for (int attempt = 0; attempt < maxRetries; ++attempt) {
		if (Shell_NotifyIcon(NIM_ADD, &nid)) {
			break;
		}
		Sleep(1000);
	}
}

void CALLBACK SystemTrayHelper::WaitToCreateFakeWindow(HWND hwnd, UINT uMsg, UINT_PTR timerId, DWORD dwTime) {
	KillTimer(0, timerId);
	_createSystemTrayIcon(ins);
}

void SystemTrayHelper::createSystemTrayIcon(const HINSTANCE& hIns) {
	_createSystemTrayIcon(hIns);
}

void SystemTrayHelper::removeSystemTray() {
	Shell_NotifyIcon(NIM_DELETE, &nid);
}
