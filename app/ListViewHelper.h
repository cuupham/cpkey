#pragma once
#include "common.h"

// Small helpers shared by the dialogs that show a two-column report list.

inline void addColumn(HWND list, int index, LPCTSTR title, int width) {
	LVCOLUMN col = { 0 };
	col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	col.pszText = (LPTSTR)title;
	col.cx = width;
	col.iSubItem = index;
	ListView_InsertColumn(list, index, &col);
}

inline int addRow(HWND list, int index, const wstring& first, const wstring& second) {
	LVITEM item = { 0 };
	item.mask = LVIF_TEXT;
	item.iItem = index;
	item.pszText = (LPTSTR)first.c_str();
	int row = ListView_InsertItem(list, &item);
	ListView_SetItemText(list, row, 1, (LPTSTR)second.c_str());
	return row;
}

inline void selectRow(HWND list, int row) {
	ListView_SetItemState(list, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	ListView_EnsureVisible(list, row, FALSE);
}
