#pragma once
#include "common.h"

// Dark look for every CPKey dialog, built without undocumented uxtheme ordinals.
namespace DarkTheme {

// Call first from each dialog procedure. Handles the colour messages and themes the dialog on
// WM_INITDIALOG; returns true with the message result in `result` when it consumed the message.
bool handleMessage(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam, INT_PTR& result);

// Section title: the dialog font in a heavier weight.
void setHeading(HWND label);

// Dimmed hint shown inside an empty, unfocused edit box. `text` must outlive the window.
void setPlaceholder(HWND edit, LPCTSTR text);

} // namespace DarkTheme
