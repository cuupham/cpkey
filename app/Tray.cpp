#include "Tray.h"
#include "App.h"

#define WM_TRAYMESSAGE (WM_USER + 1)
#define TRAY_ICONUID   100

#define WINDOW_RETRY_MS    3000
#define MAX_WINDOW_RETRIES 5
#define ICON_ADD_RETRY_MS  1000
#define MAX_ICON_ADD_TRIES 5

#define POPUP_VIET_ON_OFF 900
#define POPUP_SPELLING    901

#define POPUP_TELEX        910
#define POPUP_VNI          911
#define POPUP_SIMPLE_TELEX 912

#define POPUP_APP_RULES     990
#define POPUP_CONTROL_PANEL 1000
#define POPUP_EXIT          2000

#define MODIFY_MENU(MENU, COMMAND, DATA) ModifyMenu(MENU, COMMAND,                                     \
                                                    MF_BYCOMMAND | (DATA ? MF_CHECKED : MF_UNCHECKED), \
                                                    COMMAND,                                           \
                                                    menuData[COMMAND]);

static HMENU popupMenu;
static NOTIFYICONDATA nid;

static map<UINT, LPCTSTR> menuData = {
	{ POPUP_VIET_ON_OFF, _T("Bật Tiếng Việt") },
	{ POPUP_SPELLING, _T("Bật kiểm tra chính tả") },
	{ POPUP_TELEX, _T("Kiểu gõ Telex") },
	{ POPUP_VNI, _T("Kiểu gõ VNI") },
	{ POPUP_SIMPLE_TELEX, _T("Kiểu gõ Simple Telex") },
	{ POPUP_APP_RULES, _T("Cấu hình theo ứng dụng...") },
	{ POPUP_CONTROL_PANEL, _T("Bảng điều khiển...") },
	{ POPUP_EXIT, _T("Thoát") },
};

static void onTrayMenuCommand(UINT commandId) {
	App* app = App::getInstance();
	switch (commandId) {
		case POPUP_VIET_ON_OFF:
			toggleLanguage();
			break;
		case POPUP_SPELLING:
			app->onToggleCheckSpelling();
			break;
		case POPUP_TELEX:
			app->onInputType(vTelex);
			break;
		case POPUP_VNI:
			app->onInputType(vVNI);
			break;
		case POPUP_SIMPLE_TELEX:
			app->onInputType(vSimpleTelex1);
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

static LRESULT CALLBACK trayWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static UINT taskbarCreated;

	switch (message) {
		case WM_CREATE:
			taskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
			break;
		case WM_SHOW_CONTROL_PANEL:
			App::getInstance()->onControlPanel();
			break;
		case WM_TRAYMESSAGE:
			if (lParam == WM_LBUTTONDBLCLK) {
				App::getInstance()->onControlPanel();
			} else if (lParam == WM_LBUTTONUP) {
				toggleLanguage();
			} else if (lParam == WM_RBUTTONDOWN) {
				POINT curPoint;
				GetCursorPos(&curPoint);
				SetForegroundWindow(hWnd);
				UINT commandId = TrackPopupMenu(popupMenu, TPM_RETURNCMD | TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hWnd, NULL);
				onTrayMenuCommand(commandId);
				Tray::refresh();
			}
			break;
		default:
			// A restarted taskbar loses the icon.
			if (message == taskbarCreated) {
				Shell_NotifyIcon(NIM_ADD, &nid);
			}
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

static HWND createHiddenWindow(const HINSTANCE& hIns) {
	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = trayWindowProc;
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

static void createPopupMenu() {
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
		LoadString(GetModuleHandle(0), IDS_TRAY_TITLE_2, nid.szTip, ARRAYSIZE(nid.szTip));
	} else {
		icon = vUseGrayIcon ? IDI_ICON_STATUS_ENG_10 : IDI_ICON_STATUS_ENG;
		LoadString(GetModuleHandle(0), IDS_TRAY_TITLE, nid.szTip, ARRAYSIZE(nid.szTip));
	}
	nid.hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(icon));
}

void Tray::refresh() {
	loadTrayIcon();
	Shell_NotifyIcon(NIM_MODIFY, &nid);

	MODIFY_MENU(popupMenu, POPUP_VIET_ON_OFF, vLanguage);
	EnableMenuItem(popupMenu, POPUP_VIET_ON_OFF, MF_BYCOMMAND | (isLanguageLocked() ? MF_GRAYED : MF_ENABLED));
	MODIFY_MENU(popupMenu, POPUP_SPELLING, vCheckSpelling);
	MODIFY_MENU(popupMenu, POPUP_TELEX, vInputType == vTelex);
	MODIFY_MENU(popupMenu, POPUP_VNI, vInputType == vVNI);
	MODIFY_MENU(popupMenu, POPUP_SIMPLE_TELEX, vInputType == vSimpleTelex1);
}

static HINSTANCE ins;
static int retryCount = 0;

static void CALLBACK retryCreate(HWND hwnd, UINT uMsg, UINT_PTR timerId, DWORD dwTime) {
	KillTimer(0, timerId);
	Tray::create(ins);
}

void Tray::create(const HINSTANCE& hIns) {
	ins = hIns;
	HWND hWnd = createHiddenWindow(ins);

	// The shell may not be ready at logon; try again a few times before giving up.
	if (hWnd == NULL) {
		if (retryCount >= MAX_WINDOW_RETRIES) {
			PostQuitMessage(0);
			return;
		}
		SetTimer(NULL, 0, WINDOW_RETRY_MS, retryCreate);
		++retryCount;
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

	// Shell_NotifyIcon fails until the tray is initialized.
	for (int attempt = 0; attempt < MAX_ICON_ADD_TRIES; ++attempt) {
		if (Shell_NotifyIcon(NIM_ADD, &nid)) {
			break;
		}
		Sleep(ICON_ADD_RETRY_MS);
	}
}

void Tray::remove() {
	Shell_NotifyIcon(NIM_DELETE, &nid);
}
