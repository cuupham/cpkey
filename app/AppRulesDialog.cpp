#include "AppRulesDialog.h"
#include "AppRules.h"
#include "App.h"
#include "ListViewHelper.h"
#include "DarkTheme.h"
#include <commdlg.h>

#define WINDOW_TITLE_SIZE 256
#define APP_COLUMN_TEXT   _T("Ứng dụng")

/*---------------------- app picker ----------------------*/

struct PickState {
	HWND list;
	vector<string> exes;
	string exe;
	int mode;
};

static int findPickedApp(const PickState& state, const string& exe) {
	for (size_t i = 0; i < state.exes.size(); i++) {
		if (_stricmp(state.exes[i].c_str(), exe.c_str()) == 0) return (int)i;
	}
	return -1;
}

static int addPickedApp(PickState& state, const string& exe, const wstring& title) {
	int found = findPickedApp(state, exe);
	if (found >= 0) return found;
	state.exes.push_back(exe);
	return addRow(state.list, (int)state.exes.size() - 1, Os::toWide(exe), title);
}

static BOOL CALLBACK enumRunningApps(HWND hwnd, LPARAM lParam) {
	static const string selfExe = Os::getExecutableName(Os::getFullPath());
	TCHAR title[WINDOW_TITLE_SIZE];
	if (!IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER) ||
	    (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) || !GetWindowText(hwnd, title, WINDOW_TITLE_SIZE)) {
		return TRUE;
	}
	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);
	HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!process) return TRUE;
	TCHAR path[MAX_PATH];
	DWORD size = MAX_PATH;
	BOOL ok = QueryFullProcessImageName(process, 0, path, &size);
	CloseHandle(process);
	if (!ok) return TRUE;
	string exe = Os::getExecutableName(path);
	if (_stricmp(exe.c_str(), selfExe.c_str()) != 0) {
		addPickedApp(*(PickState*)lParam, exe, title);
	}
	return TRUE;
}

static void browseExe(HWND hDlg, PickState& state) {
	TCHAR file[MAX_PATH] = { 0 };
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFile = file;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = _T("Ứng dụng (*.exe)\0*.exe\0");
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&ofn)) {
		selectRow(state.list, addPickedApp(state, Os::getExecutableName(file), file));
	}
}

static INT_PTR CALLBACK pickAppProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	INT_PTR themed;
	if (DarkTheme::handleMessage(hDlg, uMsg, wParam, lParam, themed)) return themed;
	PickState* state = (PickState*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	switch (uMsg) {
		case WM_INITDIALOG: {
			state = (PickState*)lParam;
			SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
			state->list = GetDlgItem(hDlg, IDC_LIST_RUNNING_APPS);
			ListView_SetExtendedListViewStyle(state->list, LVS_EX_FULLROWSELECT);
			addColumn(state->list, 0, APP_COLUMN_TEXT, 110);
			addColumn(state->list, 1, _T("Cửa sổ"), 125);
			EnumWindows(enumRunningApps, (LPARAM)state);
			CheckRadioButton(hDlg, IDC_RADIO_RULE_ENGLISH, IDC_RADIO_RULE_VIETNAMESE, IDC_RADIO_RULE_ENGLISH);
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_BUTTON_BROWSE:
					browseExe(hDlg, *state);
					return TRUE;
				case IDOK: {
					int row = ListView_GetNextItem(state->list, -1, LVNI_SELECTED);
					if (row < 0) {
						MessageBox(hDlg, _T("Chọn một ứng dụng."), APP_NAME, MB_OK);
						return TRUE;
					}
					state->exe = state->exes[row];
					state->mode = IsDlgButtonChecked(hDlg, IDC_RADIO_RULE_VIETNAMESE) ? LANGUAGE_VIETNAMESE : LANGUAGE_ENGLISH;
					EndDialog(hDlg, IDOK);
					return TRUE;
				}
				case IDCANCEL:
					EndDialog(hDlg, IDCANCEL);
					return TRUE;
			}
			break;
	}
	return FALSE;
}

/*---------------------- rules form ----------------------*/

AppRulesDialog::AppRulesDialog(const HINSTANCE& instance, const int& resourceId)
    : BaseDialog(instance, resourceId) {
}

INT_PTR AppRulesDialog::eventProc(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG:
			hDlg = dialog;
			initDialog();
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					App::getInstance()->closeDialog(this);
					break;
				case IDC_BUTTON_RULE_ADD:
					onAdd();
					break;
				case IDC_BUTTON_RULE_TOGGLE:
					onToggle();
					break;
				case IDC_BUTTON_RULE_DELETE:
					onDelete();
					break;
			}
			break;
		case WM_NOTIFY: {
			LPNMHDR header = (LPNMHDR)lParam;
			if (header->idFrom == IDC_LIST_APP_RULES && header->code == NM_DBLCLK) {
				onToggle();
			}
			break;
		}
	}
	return FALSE;
}

void AppRulesDialog::initDialog() {
	setDialogIcon();
	hList = GetDlgItem(hDlg, IDC_LIST_APP_RULES);
	ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);
	addColumn(hList, 0, APP_COLUMN_TEXT, 140);
	addColumn(hList, 1, _T("Chế độ gõ"), 100);
	refresh();
}

void AppRulesDialog::refresh() {
	ListView_DeleteAllItems(hList);
	int index = 0;
	for (const auto& rule : getAllAppRules()) {
		addRow(hList, index++, Os::toWide(rule.first), ruleModeText(rule.second));
	}
}

int AppRulesDialog::selectedIndex() {
	return ListView_GetNextItem(hList, -1, LVNI_SELECTED);
}

string AppRulesDialog::appAt(int index) {
	TCHAR name[MAX_PATH];
	ListView_GetItemText(hList, index, 0, name, MAX_PATH);
	return Os::toUtf8(name);
}

void AppRulesDialog::onAdd() {
	PickState state;
	if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG_PICK_APP), hDlg, pickAppProc, (LPARAM)&state) == IDOK) {
		setAppRule(state.exe, state.mode);
		refresh();
	}
}

void AppRulesDialog::onToggle() {
	int row = selectedIndex();
	if (row < 0) return;
	string exe = appAt(row);
	setAppRule(exe, getAppRule(exe) == LANGUAGE_VIETNAMESE ? LANGUAGE_ENGLISH : LANGUAGE_VIETNAMESE);
	refresh();
	selectRow(hList, row);
}

void AppRulesDialog::onDelete() {
	int row = selectedIndex();
	if (row < 0) return;
	removeAppRule(appAt(row));
	refresh();
	int count = ListView_GetItemCount(hList);
	if (count > 0) selectRow(hList, min(row, count - 1));
}
