#include "MainControlDialog.h"
#include "AppDelegate.h"
#include "AppRules.h"
#include "QuickTyping.h"
#include "QuickTypingDialog.h"
#include <shlobj.h>

// Height of the collapsible "Nâng cao" block in dialog units; keep in sync with app.rc.
#define ADVANCED_BLOCK_HEIGHT 36

static const int _advancedControls[] = {
	IDC_CHECK_FIX_RECOMMEND_BROWSER,
	IDC_CHECK_FIX_CHROMIUM,
	IDC_CHECK_USE_CLIPBOARD,
};

// Controls below the advanced block; they move when it opens or closes.
static const int _footerControls[] = { ID_BTN_DEFAULT, IDBUTTON_OK };

#define SWITCH_KEY_CTRL_MASK  0x100
#define SWITCH_KEY_ALT_MASK   0x200
#define SWITCH_KEY_WIN_MASK   0x400
#define SWITCH_KEY_SHIFT_MASK 0x800
#define SWITCH_KEY_BEEP_MASK  0x8000

struct SwitchKeyOption {
	int controlId;
	int mask;
};

static const SwitchKeyOption _switchKeyOptions[] = {
	{ IDC_CHECK_SWITCH_KEY_CTRL, SWITCH_KEY_CTRL_MASK },
	{ IDC_CHECK_SWITCH_KEY_ALT, SWITCH_KEY_ALT_MASK },
	{ IDC_CHECK_SWITCH_KEY_WIN, SWITCH_KEY_WIN_MASK },
	{ IDC_CHECK_SWITCH_KEY_SHIFT, SWITCH_KEY_SHIFT_MASK },
	{ IDC_CHECK_SWITCH_KEY_BEEP, SWITCH_KEY_BEEP_MASK },
};

struct CheckOption {
	int controlId;
	LPCTSTR name;
	int* value;
	bool inverted;
};

#define CHECK_OPTION(controlId, setting, inverted) { controlId, _T(#setting), &setting, inverted }

static const CheckOption _checkOptions[] = {
	CHECK_OPTION(IDC_CHECK_MODERN_ORTHORGRAPHY, vUseModernOrthography, false),
	CHECK_OPTION(IDC_CHECK_FIX_RECOMMEND_BROWSER, vFixRecommendBrowser, false),
	CHECK_OPTION(IDC_CHECK_CAPS_FIRST_CHAR, vUpperCaseFirstChar, false),
	CHECK_OPTION(IDC_CHECK_QUICK_TYPING, vUseMacro, false),
	CHECK_OPTION(IDC_CHECK_RUN_WITH_WINDOWS, vRunWithWindows, false),
	CHECK_OPTION(IDC_CHECK_RUN_AS_ADMIN, vRunAsAdmin, false),
	CHECK_OPTION(IDC_CHECK_SPELLING, vCheckSpelling, false),
	CHECK_OPTION(IDC_CHECK_RESTORE_IF_WRONG_SPELLING, vRestoreIfWrongSpelling, false),
	CHECK_OPTION(IDC_CHECK_ALLOW_ZJWF, vAllowConsonantZFWJ, false),
	CHECK_OPTION(IDC_CHECK_TEMP_OFF_SPELLING, vTempOffSpelling, false),
	CHECK_OPTION(IDC_CHECK_FIX_CHROMIUM, vFixChromiumBrowser, false),
	CHECK_OPTION(IDC_CHECK_USE_CLIPBOARD, vSendKeyStepByStep, true),
	CHECK_OPTION(IDC_CHECK_MODERN_ICON, vUseGrayIcon, false),
};

// Order matches the engine's input type numbers.
static const LPCTSTR INPUT_TYPE_NAMES[] = { _T("Telex"), _T("VNI"), _T("Simple Telex") };

static void setChecked(HWND hDlg, int controlId, bool checked) {
	CheckDlgButton(hDlg, controlId, checked ? BST_CHECKED : BST_UNCHECKED);
}

MainControlDialog::MainControlDialog(const HINSTANCE& instance, const int& resourceId)
    : BaseDialog(instance, resourceId) {
}

void MainControlDialog::initDialog() {
	setDialogIcon();
	SetWindowText(hDlg, APP_NAME _T(" - Bộ gõ Tiếng Việt"));

	comboBoxInputType = GetDlgItem(hDlg, IDC_COMBO_INPUT_TYPE);
	for (LPCTSTR name : INPUT_TYPE_NAMES) {
		SendMessage(comboBoxInputType, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
	}
	textSwitchKey = GetDlgItem(hDlg, IDC_SWITCH_KEY_KEY);
	setAdvancedOpen(false);
	fillData();
}

void MainControlDialog::setAdvancedOpen(bool open) {
	RECT block = { 0, 0, 0, ADVANCED_BLOCK_HEIGHT };
	MapDialogRect(hDlg, &block);
	int shift = open == advancedOpen ? 0 : (open ? block.bottom : -block.bottom);
	advancedOpen = open;

	for (int controlId : _advancedControls) {
		ShowWindow(GetDlgItem(hDlg, controlId), open ? SW_SHOW : SW_HIDE);
	}
	for (int controlId : _footerControls) {
		HWND button = GetDlgItem(hDlg, controlId);
		RECT rect;
		GetWindowRect(button, &rect);
		MapWindowPoints(NULL, hDlg, (POINT*)&rect, 2);
		SetWindowPos(button, NULL, rect.left, rect.top + shift, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	RECT window;
	GetWindowRect(hDlg, &window);
	SetWindowPos(hDlg, NULL, 0, 0, window.right - window.left, window.bottom - window.top + shift, SWP_NOMOVE | SWP_NOZORDER);
	SetDlgItemText(hDlg, IDC_BUTTON_ADVANCED, open ? _T("Nâng cao ▾") : _T("Nâng cao ▸"));
}

void MainControlDialog::updateRulesButton() {
	size_t appRuleCount = getAllAppRules().size();
	wstring appRules = appRuleCount ? L"Quản lý (" + to_wstring(appRuleCount) + L")..." : L"Quản lý...";
	SetDlgItemText(hDlg, IDC_BUTTON_APP_RULES, appRules.c_str());

	size_t quickCount = getQuickTyping().size();
	wstring quickTyping = quickCount ? L"Tùy chỉnh (" + to_wstring(quickCount) + L")..." : L"Tùy chỉnh...";
	SetDlgItemText(hDlg, IDC_BUTTON_QUICK_TYPING, quickTyping.c_str());
}

INT_PTR MainControlDialog::eventProc(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG:
			hDlg = dialog;
			initDialog();
			// Start on the default button so Enter closes the panel instead of hitting "Cài đặt gốc...".
			SetFocus(GetDlgItem(hDlg, IDBUTTON_OK));
			return FALSE;
		case WM_COMMAND: {
			int controlId = LOWORD(wParam);
			switch (controlId) {
				case IDBUTTON_OK:
				case IDCANCEL:
					AppDelegate::getInstance()->closeDialog(this);
					break;
				case IDC_BUTTON_APP_RULES:
					AppDelegate::getInstance()->onAppRules();
					break;
				case IDC_BUTTON_QUICK_TYPING:
					showQuickTypingDialog(hInstance, hDlg);
					fillData();
					break;
				case IDC_BUTTON_ADVANCED:
					setAdvancedOpen(!advancedOpen);
					break;
				case ID_BTN_DEFAULT:
					if (MessageBox(hDlg, _T("Bạn có chắc chắn muốn thiết lập lại cài đặt gốc?"), APP_NAME,
					               MB_ICONEXCLAMATION | MB_YESNO) == IDYES) {
						AppDelegate::getInstance()->onDefaultConfig();
					}
					break;
				case IDC_RADIO_METHOD_VIETNAMESE:
					setBaseLanguage(1);
					break;
				case IDC_RADIO_METHOD_ENGLISH:
					setBaseLanguage(0);
					break;
				default:
					if (HIWORD(wParam) == CBN_SELCHANGE && controlId == IDC_COMBO_INPUT_TYPE) {
						AppDelegate::getInstance()->onInputType((int)SendMessage(comboBoxInputType, CB_GETCURSEL, 0, 0));
					} else if (HIWORD(wParam) == BN_CLICKED) {
						onCheckboxClicked(controlId);
					} else if (HIWORD(wParam) == EN_CHANGE && controlId == IDC_SWITCH_KEY_KEY) {
						UINT16 keyCode = lastKeyCode;
						if (keyCode > 0) {
							lastKeyCode = 0;
							onCharacter(keyCode);
						}
					}
					break;
			}
			break;
		}
	}
	return FALSE;
}

void MainControlDialog::fillData() {
	updateRulesButton();
	SendMessage(comboBoxInputType, CB_SETCURSEL, vInputType, 0);

	for (const SwitchKeyOption& option : _switchKeyOptions) {
		setChecked(hDlg, option.controlId, vSwitchKeyStatus & option.mask);
	}
	setSwitchKeyText((vSwitchKeyStatus >> 24) & 0xFF);

	setChecked(hDlg, IDC_RADIO_METHOD_VIETNAMESE, getBaseLanguage());
	setChecked(hDlg, IDC_RADIO_METHOD_ENGLISH, !getBaseLanguage());

	for (const CheckOption& option : _checkOptions) {
		setChecked(hDlg, option.controlId, (*option.value != 0) != option.inverted);
	}

	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_RESTORE_IF_WRONG_SPELLING), vCheckSpelling);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_ALLOW_ZJWF), vCheckSpelling);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_TEMP_OFF_SPELLING), vCheckSpelling);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_FIX_CHROMIUM), vFixRecommendBrowser);
}

void MainControlDialog::onCheckboxClicked(int controlId) {
	bool checked = IsDlgButtonChecked(hDlg, controlId) == BST_CHECKED;

	for (const SwitchKeyOption& option : _switchKeyOptions) {
		if (option.controlId == controlId) {
			APP_SET_DATA(vSwitchKeyStatus, checked ? (vSwitchKeyStatus | option.mask) : (vSwitchKeyStatus & ~option.mask));
			return;
		}
	}

	for (const CheckOption& option : _checkOptions) {
		if (option.controlId != controlId) continue;
		*option.value = checked != option.inverted;
		SystemHelper::setIntSetting(option.name, *option.value);
		if (option.value == &vCheckSpelling) {
			vSetCheckSpelling();
		} else if (option.value == &vRunWithWindows) {
			SystemHelper::registerRunOnStartup(vRunWithWindows);
		} else if (option.value == &vRunAsAdmin) {
			requestRestartAsAdmin();
		}
		fillData();
		SystemTrayHelper::updateData();
		return;
	}
}

void MainControlDialog::onCharacter(const UINT16& keyCode) {
	UINT16 code = GET_SWITCH_KEY(vSwitchKeyStatus);
	if (keyCode == VK_DELETE || keyCode == VK_BACK) {
		code = 0xFE;
	} else if (keyCodeToCharacter(keyCode) != 0) {
		code = keyCode;
	}
	// The key lives in both the low byte and the top byte of the setting.
	vSwitchKeyStatus = (vSwitchKeyStatus & 0x00FFFF00) | code | ((unsigned int)code << 24);
	SystemHelper::setIntSetting(_T("vSwitchKeyStatus"), vSwitchKeyStatus);
	setSwitchKeyText(code);
}

void MainControlDialog::setSwitchKeyText(const UINT16& keyCode) {
	if (keyCode == KEY_SPACE) {
		SetWindowText(textSwitchKey, _T("Space"));
	} else if (keyCode == 0xFE) {
		SetWindowText(textSwitchKey, _T(""));
	} else {
		Uint16 key[] = { keyCode, 0 };
		SetWindowText(textSwitchKey, (LPCWSTR)&key);
	}
}

void MainControlDialog::requestRestartAsAdmin() {
	if (vRunAsAdmin && !IsUserAnAdmin()) {
		if (MessageBox(hDlg, _T("Bạn cần khởi động lại để chạy với quyền Admin.\nKhởi động lại ngay bây giờ?"), APP_NAME,
		               MB_ICONEXCLAMATION | MB_YESNO) == IDYES) {
			// The elevated instance registers autostart again when it starts.
			SystemHelper::registerRunOnStartup(false);
			// Quit first: the new instance exits if it still finds this one running.
			AppDelegate::getInstance()->onExit();
			ShellExecute(0, L"runas", SystemHelper::getFullPath().c_str(), 0, 0, SW_SHOWNORMAL);
			return;
		}
		// Declined: stay in the current mode so autostart keeps working.
		vRunAsAdmin = 0;
		SystemHelper::setIntSetting(_T("vRunAsAdmin"), 0);
	}
	SystemHelper::registerRunOnStartup(false);
	if (vRunWithWindows) {
		SystemHelper::registerRunOnStartup(true);
	}
}
