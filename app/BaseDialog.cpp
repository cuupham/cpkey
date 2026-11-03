#include "BaseDialog.h"
#include "DarkTheme.h"

INT_PTR CALLBACK BaseDialog::dialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	INT_PTR themed;
	if (DarkTheme::handleMessage(hDlg, uMsg, wParam, lParam, themed)) return themed;
	if (uMsg == WM_INITDIALOG) {
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
	}
	BaseDialog* self = (BaseDialog*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	return self ? self->eventProc(hDlg, uMsg, wParam, lParam) : FALSE;
}

BaseDialog::BaseDialog(const HINSTANCE& instance, const int& resourceId) {
	resId = resourceId;
	hInstance = instance;
}

void BaseDialog::setDialogIcon() {
	HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
	if (hIcon) {
		SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}
}

void BaseDialog::show() {
	hDlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(resId), 0, dialogProc, (LPARAM)this);
	ShowWindow(hDlg, SW_SHOWNORMAL);
}

void BaseDialog::bringOnTop() {
	SetForegroundWindow(hDlg);
	SetActiveWindow(hDlg);
}

HWND BaseDialog::getHwnd() {
	return hDlg;
}

void BaseDialog::closeDialog() {
	SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
	DestroyWindow(hDlg);
}
