#include "QuickTypingDialog.h"
#include "QuickTyping.h"
#include "ListViewHelper.h"
#include "DarkTheme.h"

static HWND listOf(HWND hDlg) {
	return GetDlgItem(hDlg, IDC_LIST_QUICK_TYPING);
}

static wstring editText(HWND hDlg, int controlId) {
	TCHAR text[TEXT_BUFFER] = { 0 };
	GetDlgItemText(hDlg, controlId, text, TEXT_BUFFER);
	return wstring(text);
}

static int rowOf(HWND hDlg, const wstring& trigger) {
	HWND list = listOf(hDlg);
	for (int row = 0; row < ListView_GetItemCount(list); row++) {
		TCHAR text[TEXT_BUFFER] = { 0 };
		ListView_GetItemText(list, row, 0, text, TEXT_BUFFER);
		if (trigger == text) return row;
	}
	return -1;
}

static wstring selectedTrigger(HWND hDlg) {
	HWND list = listOf(hDlg);
	int row = ListView_GetNextItem(list, -1, LVNI_SELECTED);
	if (row < 0) return L"";
	TCHAR text[TEXT_BUFFER] = { 0 };
	ListView_GetItemText(list, row, 0, text, TEXT_BUFFER);
	return wstring(text);
}

static void updateSetButton(HWND hDlg) {
	SetDlgItemText(hDlg, IDC_BUTTON_QUICK_SET, hasQuickTyping(editText(hDlg, IDC_EDIT_QUICK_TRIGGER)) ? _T("Sửa") : _T("Thêm"));
}

static void updateButtons(HWND hDlg) {
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_QUICK_DELETE), !selectedTrigger(hDlg).empty());
	updateSetButton(hDlg);
}

static void showMessages(HWND hDlg, const wstring& triggerMessage, const wstring& outputMessage) {
	SetDlgItemText(hDlg, IDC_STATIC_QUICK_TRIGGER_MSG, triggerMessage.c_str());
	SetDlgItemText(hDlg, IDC_STATIC_QUICK_OUTPUT_MSG, outputMessage.c_str());
}

// A trigger that already exists is not an error: the Add button just becomes Edit.
static wstring triggerMessage(HWND hDlg) {
	wstring trigger = editText(hDlg, IDC_EDIT_QUICK_TRIGGER);
	wstring error = checkQuickTrigger(trigger);
	if (error.empty() && hasQuickTyping(trigger)) return L"Đã có. Bấm Sửa để đổi nội dung.";
	return error;
}

static void refreshList(HWND hDlg) {
	HWND list = listOf(hDlg);
	ListView_DeleteAllItems(list);
	int index = 0;
	for (const auto& rule : getQuickTyping()) {
		addRow(list, index++, rule.first, rule.second);
	}
	int visible = index == 0 ? SW_SHOW : SW_HIDE;
	ShowWindow(GetDlgItem(hDlg, IDC_STATIC_QUICK_EMPTY), visible);
	ShowWindow(GetDlgItem(hDlg, IDC_BUTTON_QUICK_EXAMPLE), visible);
	updateButtons(hDlg);
}

static void clearForm(HWND hDlg) {
	SetDlgItemText(hDlg, IDC_EDIT_QUICK_TRIGGER, _T(""));
	SetDlgItemText(hDlg, IDC_EDIT_QUICK_OUTPUT, _T(""));
	showMessages(hDlg, L"", L"");
}

static void onSet(HWND hDlg) {
	wstring trigger = editText(hDlg, IDC_EDIT_QUICK_TRIGGER), output = editText(hDlg, IDC_EDIT_QUICK_OUTPUT);
	wstring triggerError = checkQuickTrigger(trigger), outputError = checkQuickOutput(output);
	showMessages(hDlg, triggerError, outputError);
	if (!triggerError.empty()) {
		SetFocus(GetDlgItem(hDlg, IDC_EDIT_QUICK_TRIGGER));
		return;
	}
	if (!outputError.empty()) {
		SetFocus(GetDlgItem(hDlg, IDC_EDIT_QUICK_OUTPUT));
		return;
	}
	setQuickTyping(trigger, output);
	refreshList(hDlg);
	selectRow(listOf(hDlg), rowOf(hDlg, trigger));
	showMessages(hDlg, L"", L"");
}

static void onDelete(HWND hDlg) {
	wstring trigger = selectedTrigger(hDlg);
	if (trigger.empty()) return;
	removeQuickTyping(trigger);
	refreshList(hDlg);
	clearForm(hDlg);
}

static void onNew(HWND hDlg) {
	ListView_SetItemState(listOf(hDlg), -1, 0, LVIS_SELECTED);
	clearForm(hDlg);
	updateButtons(hDlg);
	SetFocus(GetDlgItem(hDlg, IDC_EDIT_QUICK_TRIGGER));
}

static void onExample(HWND hDlg) {
	SetDlgItemText(hDlg, IDC_EDIT_QUICK_TRIGGER, _T("vn"));
	SetDlgItemText(hDlg, IDC_EDIT_QUICK_OUTPUT, _T("Việt Nam"));
	updateSetButton(hDlg);
	SetFocus(GetDlgItem(hDlg, IDC_BUTTON_QUICK_SET));
}

static INT_PTR CALLBACK quickTypingProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	INT_PTR themed;
	if (DarkTheme::handleMessage(hDlg, uMsg, wParam, lParam, themed)) return themed;
	switch (uMsg) {
		case WM_INITDIALOG: {
			HICON icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON));
			SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)icon);
			HWND list = listOf(hDlg);
			ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT);
			addColumn(list, 0, _T("Gõ"), 90);
			addColumn(list, 1, _T("Ra"), 160);
			SendDlgItemMessage(hDlg, IDC_EDIT_QUICK_TRIGGER, EM_LIMITTEXT, QUICK_TYPING_MAX_TRIGGER, 0);
			SendDlgItemMessage(hDlg, IDC_EDIT_QUICK_OUTPUT, EM_LIMITTEXT, QUICK_TYPING_MAX_OUTPUT, 0);
			refreshList(hDlg);
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_BUTTON_QUICK_SET:
					onSet(hDlg);
					return TRUE;
				case IDC_BUTTON_QUICK_DELETE:
					onDelete(hDlg);
					return TRUE;
				case IDC_BUTTON_QUICK_NEW:
					onNew(hDlg);
					return TRUE;
				case IDC_BUTTON_QUICK_EXAMPLE:
					onExample(hDlg);
					return TRUE;
				case IDC_EDIT_QUICK_TRIGGER:
					if (HIWORD(wParam) == EN_CHANGE) updateSetButton(hDlg);
					if (HIWORD(wParam) == EN_KILLFOCUS && !editText(hDlg, IDC_EDIT_QUICK_TRIGGER).empty()) {
						SetDlgItemText(hDlg, IDC_STATIC_QUICK_TRIGGER_MSG, triggerMessage(hDlg).c_str());
					}
					return TRUE;
				case IDC_EDIT_QUICK_OUTPUT:
					if (HIWORD(wParam) == EN_KILLFOCUS && !editText(hDlg, IDC_EDIT_QUICK_OUTPUT).empty()) {
						SetDlgItemText(hDlg, IDC_STATIC_QUICK_OUTPUT_MSG, checkQuickOutput(editText(hDlg, IDC_EDIT_QUICK_OUTPUT)).c_str());
					}
					return TRUE;
				case IDCANCEL:
					EndDialog(hDlg, IDOK);
					return TRUE;
			}
			break;
		case WM_NOTIFY: {
			LPNMHDR header = (LPNMHDR)lParam;
			if (header->idFrom != IDC_LIST_QUICK_TYPING) break;
			if (header->code == LVN_ITEMCHANGED && (((LPNMLISTVIEW)lParam)->uNewState & LVIS_SELECTED)) {
				int row = ((LPNMLISTVIEW)lParam)->iItem;
				TCHAR trigger[TEXT_BUFFER] = { 0 }, output[TEXT_BUFFER] = { 0 };
				ListView_GetItemText(listOf(hDlg), row, 0, trigger, TEXT_BUFFER);
				ListView_GetItemText(listOf(hDlg), row, 1, output, TEXT_BUFFER);
				SetDlgItemText(hDlg, IDC_EDIT_QUICK_TRIGGER, trigger);
				SetDlgItemText(hDlg, IDC_EDIT_QUICK_OUTPUT, output);
				showMessages(hDlg, L"", L"");
				updateButtons(hDlg);
			} else if (header->code == LVN_KEYDOWN && ((LPNMLVKEYDOWN)lParam)->wVKey == VK_DELETE) {
				onDelete(hDlg);
			}
			break;
		}
	}
	return FALSE;
}

void showQuickTypingDialog(HINSTANCE hInstance, HWND parent) {
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_QUICK_TYPING), parent, quickTypingProc);
}
