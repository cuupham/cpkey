#include "AppDelegate.h"
#include "Settings.h"

static AppDelegate* _instance;

bool AppDelegate::isDialogMsg(MSG& msg) const {
	return (mainDialog != NULL && IsDialogMessage(mainDialog->getHwnd(), &msg)) ||
	       (rulesDialog != NULL && IsDialogMessage(rulesDialog->getHwnd(), &msg));
}

AppDelegate::AppDelegate() {
	_instance = this;
}

AppDelegate* AppDelegate::getInstance() {
	return _instance;
}

int AppDelegate::run(HINSTANCE instance) {
	hInstance = instance;

	//only one instance: ask the running one to open its control panel
	HWND previousInstance = FindWindow(APP_CLASS, NULL);
	if (previousInstance) {
		SendMessage(previousInstance, WM_USER + 2019, 0, 0);
		return 0;
	}

	INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_LISTVIEW_CLASSES };
	InitCommonControlsEx(&controls);

	keyHookInit();
	SystemTrayHelper::createSystemTrayIcon(hInstance);
	applyAppRule();
	SystemTrayHelper::updateData();

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (msg.message == WM_KEYDOWN) {
			lastKeyCode = (UINT16)msg.wParam;
		}
		if (!isDialogMsg(msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}

void AppDelegate::createMainDialog() {
	if (mainDialog == NULL) {
		mainDialog = new MainControlDialog(hInstance, IDD_DIALOG_MAIN);
		mainDialog->show();
	} else {
		mainDialog->bringOnTop();
	}
}

void AppDelegate::closeDialog(BaseDialog* dialog) {
	dialog->closeDialog();
	if (mainDialog == dialog) {
		mainDialog = NULL;
	} else if (rulesDialog == dialog) {
		rulesDialog = NULL;
	}
	delete dialog;
}

void AppDelegate::refreshUi() {
	if (mainDialog) {
		mainDialog->fillData();
	}
	SystemTrayHelper::updateData();
}

void AppDelegate::onDefaultConfig() {
#define RESET_SETTING(name, value) APP_SET_DATA(name, value);
	PERSISTED_SETTINGS(RESET_SETTING)
	setBaseLanguage(1);
	refreshUi();
}

void AppDelegate::onToggleVietnamese() {
	toggleLanguage();
}

void AppDelegate::onToggleCheckSpelling() {
	APP_SET_DATA(vCheckSpelling, vCheckSpelling ? 0 : 1);
	vSetCheckSpelling();
	refreshUi();
}

void AppDelegate::onInputType(const int& type) {
	APP_SET_DATA(vInputType, type);
	refreshUi();
}

void AppDelegate::onAppRules() {
	if (rulesDialog == NULL) {
		rulesDialog = new AppRulesDialog(hInstance, IDD_DIALOG_APP_RULES);
		rulesDialog->show();
	} else {
		rulesDialog->bringOnTop();
	}
}

void AppDelegate::onControlPanel() {
	createMainDialog();
}

void AppDelegate::onExit() {
	keyHookFree();
	SystemTrayHelper::removeSystemTray();
	PostQuitMessage(0);
}
