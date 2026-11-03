#include "App.h"
#include "Settings.h"

static App* current;

bool App::isDialogMsg(MSG& msg) const {
	return (mainDialog != NULL && IsDialogMessage(mainDialog->getHwnd(), &msg)) ||
	       (rulesDialog != NULL && IsDialogMessage(rulesDialog->getHwnd(), &msg));
}

App::App() {
	current = this;
}

App* App::getInstance() {
	return current;
}

int App::run(HINSTANCE instance) {
	hInstance = instance;

	// Single instance: ask the running one to open its control panel.
	HWND previousInstance = FindWindow(APP_CLASS, NULL);
	if (previousInstance) {
		SendMessage(previousInstance, WM_SHOW_CONTROL_PANEL, 0, 0);
		return 0;
	}

	INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES };
	InitCommonControlsEx(&controls);

	keyHookInit();
	Tray::create(hInstance);
	applyAppRule();
	Tray::refresh();

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

void App::closeDialog(BaseDialog* dialog) {
	dialog->closeDialog();
	if (mainDialog == dialog) {
		mainDialog = NULL;
	} else if (rulesDialog == dialog) {
		rulesDialog = NULL;
	}
	delete dialog;
}

void App::refreshUi() {
	if (mainDialog) {
		mainDialog->refresh();
	}
	Tray::refresh();
}

void App::onDefaultConfig() {
#define RESET_SETTING(name, value) APP_SET_DATA(name, value);
	PERSISTED_SETTINGS(RESET_SETTING)
	setBaseLanguage(LANGUAGE_VIETNAMESE);
	refreshUi();
}

void App::onToggleCheckSpelling() {
	APP_SET_DATA(vCheckSpelling, vCheckSpelling ? 0 : 1);
	vSetCheckSpelling();
	refreshUi();
}

void App::onInputType(const int& type) {
	APP_SET_DATA(vInputType, type);
	refreshUi();
}

void App::onAppRules() {
	if (rulesDialog == NULL) {
		rulesDialog = new AppRulesDialog(hInstance, IDD_DIALOG_APP_RULES);
		rulesDialog->show();
	} else {
		rulesDialog->bringOnTop();
	}
}

void App::onControlPanel() {
	if (mainDialog == NULL) {
		mainDialog = new MainControlDialog(hInstance, IDD_DIALOG_MAIN);
		mainDialog->show();
	} else {
		mainDialog->bringOnTop();
	}
}

void App::onExit() {
	keyHookFree();
	Tray::remove();
	PostQuitMessage(0);
}
