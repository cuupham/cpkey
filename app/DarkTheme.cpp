#include "DarkTheme.h"
#include <dwmapi.h>
#include <uxtheme.h>
#include <windowsx.h>

namespace DarkTheme {
namespace {

const COLORREF BACKGROUND = RGB(0x20, 0x20, 0x20);
const COLORREF CARD = RGB(0x2b, 0x2b, 0x2b);
const COLORREF CARD_EDGE = RGB(0x3d, 0x3d, 0x3d);
const COLORREF FIELD = RGB(0x38, 0x38, 0x38);
const COLORREF FIELD_HOVER = RGB(0x40, 0x40, 0x40);
const COLORREF FIELD_EDGE = RGB(0x48, 0x48, 0x48);
const COLORREF CONTROL_EDGE = RGB(0x8d, 0x8d, 0x8d);
const COLORREF BUTTON = RGB(0x33, 0x33, 0x33);
const COLORREF BUTTON_HOVER = RGB(0x3c, 0x3c, 0x3c);
const COLORREF BUTTON_PRESSED = RGB(0x2c, 0x2c, 0x2c);
const COLORREF TEXT_PRIMARY = RGB(0xff, 0xff, 0xff);
const COLORREF TEXT_MUTED = RGB(0xa8, 0xa8, 0xa8);
const COLORREF TEXT_DISABLED = RGB(0x6e, 0x6e, 0x6e);
const COLORREF ACCENT = RGB(0x60, 0xcd, 0xff);
const COLORREF ON_ACCENT = RGB(0x0b, 0x0b, 0x0b);
const COLORREF DISABLED_FILL = RGB(0x26, 0x26, 0x26);
const COLORREF DISABLED_EDGE = RGB(0x4a, 0x4a, 0x4a);
const COLORREF NO_FILL = CLR_NONE;

// Design pixels at 96 DPI.
const int RADIUS = 4;
const int CARD_RADIUS = 8;
const int TOGGLE_SIZE = 16;
const int TOGGLE_GAP = 6;
const int CONTROL_PADDING = 8;
const int FOCUS_RING = 2;
const int EDIT_PADDING = 3;
const int COMBO_CHEVRON_INSET = 14;
const int CHEVRON_HALF_WIDTH = 4;
const int STROKE_WIDTH = 2;
const int CAPTION_GAP = 3;

// dwmapi.h of older SDKs lacks these Windows 11 attributes; unknown ones are ignored by DWM.
const DWORD DWM_USE_DARK_TITLE = 20;
const DWORD DWM_BORDER_COLOR = 34;
const DWORD DWM_CAPTION_COLOR = 35;
const DWORD DWM_TEXT_COLOR = 36;

const UINT_PTR SUBCLASS_ID = 1;
// Set on dialogs this module themed, so every other message passes through untouched.
const TCHAR THEMED_PROPERTY[] = _T("CPKeyDarkTheme");

enum class Kind { Check,
                  Radio,
                  Push,
                  Card,
                  Combo,
                  Edit,
                  List,
                  Heading };

struct State {
	Kind kind;
	bool hot;
	HFONT ownedFont;
	LPCTSTR placeholder;
	UINT labelCommand = 0; // check box only: sent to the parent when the label is clicked
	bool labelPressed = false;
};

HBRUSH brushOf(COLORREF color) {
	static map<COLORREF, HBRUSH> cache;
	HBRUSH& brush = cache[color];
	if (!brush) brush = CreateSolidBrush(color);
	return brush;
}

int scaled(HWND window, int pixels) {
	return MulDiv(pixels, GetDpiForWindow(window), USER_DEFAULT_SCREEN_DPI);
}

bool hasClass(HWND window, LPCTSTR className) {
	TCHAR name[32];
	GetClassName(window, name, ARRAYSIZE(name));
	return lstrcmpi(name, className) == 0;
}

// An empty group box is a card: a rounded panel that other controls sit on.
bool isCard(HWND window) {
	return hasClass(window, _T("Button")) && (GetWindowLong(window, GWL_STYLE) & BS_TYPEMASK) == BS_GROUPBOX &&
	       GetWindowTextLength(window) == 0;
}

COLORREF surfaceBehind(HWND control) {
	RECT rect;
	GetWindowRect(control, &rect);
	POINT center = { (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2 };
	for (HWND sibling = GetWindow(GetParent(control), GW_CHILD); sibling; sibling = GetWindow(sibling, GW_HWNDNEXT)) {
		if (sibling == control || !IsWindowVisible(sibling) || !isCard(sibling)) continue;
		GetWindowRect(sibling, &rect);
		if (PtInRect(&rect, center)) return CARD;
	}
	return BACKGROUND;
}

bool focusVisible(HWND window) {
	return GetFocus() == window && !(SendMessage(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS);
}

void fillRounded(HDC dc, RECT rect, int radius, COLORREF fill, COLORREF edge, int edgeWidth) {
	HPEN pen = CreatePen(PS_INSIDEFRAME, edgeWidth, edge);
	HGDIOBJ oldPen = SelectObject(dc, pen);
	HGDIOBJ oldBrush = SelectObject(dc, fill == NO_FILL ? GetStockObject(NULL_BRUSH) : brushOf(fill));
	RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius * 2, radius * 2);
	SelectObject(dc, oldBrush);
	SelectObject(dc, oldPen);
	DeleteObject(pen);
}

void drawText(HDC dc, LPCTSTR text, RECT rect, COLORREF color, UINT format) {
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, color);
	DrawText(dc, text, -1, &rect, format | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
}

void strokePolyline(HWND window, HDC dc, const POINT* points, int count, COLORREF color) {
	LOGBRUSH solid = { BS_SOLID, color, 0 };
	HPEN pen = ExtCreatePen(PS_GEOMETRIC | PS_ENDCAP_ROUND | PS_JOIN_ROUND, scaled(window, STROKE_WIDTH), &solid, 0, NULL);
	HGDIOBJ oldPen = SelectObject(dc, pen);
	Polyline(dc, points, count);
	SelectObject(dc, oldPen);
	DeleteObject(pen);
}

// Draws into an off-screen bitmap so the control never flickers.
template <typename Draw>
void paintBuffered(HWND window, Draw draw) {
	PAINTSTRUCT paint;
	HDC dc = BeginPaint(window, &paint);
	RECT rect;
	GetClientRect(window, &rect);
	HDC memory = CreateCompatibleDC(dc);
	HBITMAP bitmap = CreateCompatibleBitmap(dc, rect.right, rect.bottom);
	HGDIOBJ oldBitmap = SelectObject(memory, bitmap);
	FillRect(memory, &rect, brushOf(surfaceBehind(window)));
	HFONT font = (HFONT)SendMessage(window, WM_GETFONT, 0, 0);
	if (font) SelectObject(memory, font);
	draw(memory, rect);
	BitBlt(dc, 0, 0, rect.right, rect.bottom, memory, 0, 0, SRCCOPY);
	SelectObject(memory, oldBitmap);
	DeleteObject(bitmap);
	DeleteDC(memory);
	EndPaint(window, &paint);
}

RECT toggleBox(HWND window, RECT rect) {
	int size = scaled(window, TOGGLE_SIZE);
	int ring = scaled(window, FOCUS_RING);
	int top = (rect.bottom - size) / 2;
	return { ring, top, ring + size, top + size };
}

POINT pointOf(LPARAM position) {
	return { GET_X_LPARAM(position), GET_Y_LPARAM(position) };
}

// Clicks on the label of a check box do not toggle it: only the box (with its focus ring) does.
bool onToggleBox(HWND window, POINT position) {
	RECT client;
	GetClientRect(window, &client);
	RECT hit = toggleBox(window, client);
	int ring = scaled(window, FOCUS_RING);
	InflateRect(&hit, ring, ring);
	return PtInRect(&hit, position);
}

void paintToggle(HWND window, HDC dc, RECT rect, bool radio, bool underlineLabel) {
	bool enabled = IsWindowEnabled(window);
	bool checked = SendMessage(window, BM_GETCHECK, 0, 0) == BST_CHECKED;
	int size = scaled(window, TOGGLE_SIZE);
	int ring = scaled(window, FOCUS_RING);
	RECT box = toggleBox(window, rect);

	COLORREF fill = checked ? (enabled ? ACCENT : DISABLED_EDGE) : (enabled ? FIELD : DISABLED_FILL);
	COLORREF edge = checked ? fill : (enabled ? CONTROL_EDGE : DISABLED_EDGE);
	fillRounded(dc, box, radio ? size / 2 : scaled(window, RADIUS), fill, edge, 1);

	if (checked && radio) {
		RECT dot = box;
		InflateRect(&dot, -size * 3 / 10, -size * 3 / 10);
		fillRounded(dc, dot, (dot.right - dot.left) / 2, ON_ACCENT, ON_ACCENT, 1);
	} else if (checked) {
		POINT mark[] = {
			{ box.left + size * 22 / 100, box.top + size * 52 / 100 },
			{ box.left + size * 42 / 100, box.top + size * 72 / 100 },
			{ box.left + size * 78 / 100, box.top + size * 30 / 100 },
		};
		strokePolyline(window, dc, mark, ARRAYSIZE(mark), ON_ACCENT);
	}
	if (focusVisible(window)) {
		RECT around = box;
		InflateRect(&around, ring, ring);
		fillRounded(dc, around, radio ? (around.right - around.left) / 2 : scaled(window, RADIUS) + ring, NO_FILL, ACCENT, ring);
	}

	// Text after a tab is a dimmed detail: "Label<TAB>explanation".
	TCHAR text[TEXT_BUFFER];
	GetWindowText(window, text, TEXT_BUFFER);
	LPTSTR detail = _tcschr(text, _T('\t'));
	if (detail) *detail++ = 0;
	RECT label = { box.right + scaled(window, TOGGLE_GAP), 0, rect.right, rect.bottom };
	HFONT underlined = NULL;
	HGDIOBJ regular = NULL;
	if (underlineLabel) {
		LOGFONT font;
		GetObject(GetCurrentObject(dc, OBJ_FONT), sizeof(font), &font);
		font.lfUnderline = TRUE;
		underlined = CreateFontIndirect(&font);
		regular = SelectObject(dc, underlined);
	}
	drawText(dc, text, label, enabled ? TEXT_PRIMARY : TEXT_DISABLED, DT_VCENTER);
	if (underlined) {
		SelectObject(dc, regular);
		DeleteObject(underlined);
	}
	if (detail) {
		SIZE extent;
		GetTextExtentPoint32(dc, text, lstrlen(text), &extent);
		label.left += extent.cx + scaled(window, TOGGLE_GAP);
		drawText(dc, detail, label, enabled ? TEXT_MUTED : TEXT_DISABLED, DT_VCENTER);
	}
}

void paintPush(HWND window, HDC dc, RECT rect, const State& state) {
	bool enabled = IsWindowEnabled(window);
	bool pressed = SendMessage(window, BM_GETSTATE, 0, 0) & BST_PUSHED;
	bool emphasized = enabled && ((SendMessage(window, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON) || focusVisible(window));
	COLORREF fill = pressed ? BUTTON_PRESSED : (state.hot && enabled ? BUTTON_HOVER : BUTTON);
	fillRounded(dc, rect, scaled(window, RADIUS), fill, emphasized ? ACCENT : FIELD_EDGE, scaled(window, emphasized ? 2 : 1));
	TCHAR text[TEXT_BUFFER];
	GetWindowText(window, text, TEXT_BUFFER);
	drawText(dc, text, rect, enabled ? TEXT_PRIMARY : TEXT_DISABLED, DT_CENTER | DT_VCENTER);
}

// Without a title the group box is a filled card; with one it is a plain titled frame.
void paintCard(HWND window, HDC dc, RECT rect) {
	TCHAR text[TEXT_BUFFER];
	GetWindowText(window, text, TEXT_BUFFER);
	if (!*text) {
		fillRounded(dc, rect, scaled(window, CARD_RADIUS), CARD, CARD_EDGE, 1);
		return;
	}
	SIZE extent;
	GetTextExtentPoint32(dc, text, lstrlen(text), &extent);
	RECT frame = rect;
	frame.top += extent.cy / 2;
	fillRounded(dc, frame, scaled(window, RADIUS), NO_FILL, CARD_EDGE, 1);
	int inset = scaled(window, CONTROL_PADDING);
	int margin = scaled(window, CAPTION_GAP);
	RECT gap = { inset - margin, rect.top, inset + extent.cx + margin, rect.top + extent.cy };
	FillRect(dc, &gap, brushOf(BACKGROUND));
	drawText(dc, text, { inset, rect.top, rect.right, rect.top + extent.cy }, TEXT_PRIMARY, DT_LEFT);
}

void paintCombo(HWND window, HDC dc, RECT rect, const State& state) {
	bool enabled = IsWindowEnabled(window);
	bool open = SendMessage(window, CB_GETDROPPEDSTATE, 0, 0);
	bool active = open || GetFocus() == window;
	fillRounded(dc, rect, scaled(window, RADIUS), state.hot && enabled ? FIELD_HOVER : FIELD, active ? ACCENT : FIELD_EDGE, 1);

	TCHAR text[TEXT_BUFFER] = { 0 };
	int selected = (int)SendMessage(window, CB_GETCURSEL, 0, 0);
	if (selected >= 0 && SendMessage(window, CB_GETLBTEXTLEN, selected, 0) < TEXT_BUFFER) {
		SendMessage(window, CB_GETLBTEXT, selected, (LPARAM)text);
	}
	int pad = scaled(window, CONTROL_PADDING);
	RECT label = { rect.left + pad, rect.top, rect.right - scaled(window, COMBO_CHEVRON_INSET) - pad, rect.bottom };
	drawText(dc, text, label, enabled ? TEXT_PRIMARY : TEXT_DISABLED, DT_VCENTER);

	int half = scaled(window, CHEVRON_HALF_WIDTH);
	int x = rect.right - scaled(window, COMBO_CHEVRON_INSET);
	int y = (rect.top + rect.bottom) / 2;
	POINT chevron[] = { { x - half, y - half / 2 }, { x, y + half / 2 }, { x + half, y - half / 2 } };
	strokePolyline(window, dc, chevron, ARRAYSIZE(chevron), enabled ? TEXT_MUTED : TEXT_DISABLED);
}

void paintEditPlaceholder(HWND window, const State& state) {
	if (!state.placeholder || GetWindowTextLength(window) > 0 || GetFocus() == window) return;
	HDC dc = GetDC(window);
	RECT rect;
	GetClientRect(window, &rect);
	HGDIOBJ oldFont = SelectObject(dc, (HFONT)SendMessage(window, WM_GETFONT, 0, 0));
	bool centered = GetWindowLong(window, GWL_STYLE) & ES_CENTER;
	drawText(dc, state.placeholder, rect, TEXT_MUTED, centered ? DT_CENTER : DT_LEFT);
	SelectObject(dc, oldFont);
	ReleaseDC(window, dc);
}

// The edit's border and padding live in the non-client area so the rounded frame stays intact.
void resizeEditClient(HWND window, RECT* rect) {
	HDC dc = GetDC(window);
	HGDIOBJ oldFont = SelectObject(dc, (HFONT)SendMessage(window, WM_GETFONT, 0, 0));
	TEXTMETRIC metrics;
	GetTextMetrics(dc, &metrics);
	SelectObject(dc, oldFont);
	ReleaseDC(window, dc);
	int pad = scaled(window, EDIT_PADDING);
	InflateRect(rect, -pad, -max(pad, (int)(rect->bottom - rect->top - metrics.tmHeight) / 2));
}

void paintEditFrame(HWND window) {
	HDC dc = GetWindowDC(window);
	RECT frame, client;
	GetWindowRect(window, &frame);
	GetClientRect(window, &client);
	MapWindowPoints(window, NULL, (POINT*)&client, 2);
	OffsetRect(&client, -frame.left, -frame.top);
	OffsetRect(&frame, -frame.left, -frame.top);
	ExcludeClipRect(dc, client.left, client.top, client.right, client.bottom);
	FillRect(dc, &frame, brushOf(surfaceBehind(window)));
	fillRounded(dc, frame, scaled(window, RADIUS), FIELD, GetFocus() == window ? ACCENT : FIELD_EDGE, 1);
	ReleaseDC(window, dc);
}

void redraw(HWND window) {
	RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_UPDATENOW);
}

LRESULT CALLBACK controlProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data) {
	State* state = (State*)data;
	switch (message) {
		case WM_NCDESTROY:
			RemoveWindowSubclass(window, controlProc, id);
			if (state->ownedFont) DeleteObject(state->ownedFont);
			delete state;
			return DefSubclassProc(window, message, wParam, lParam);
		case WM_NCCALCSIZE:
			if (state->kind == Kind::Edit) {
				resizeEditClient(window, wParam ? &((NCCALCSIZE_PARAMS*)lParam)->rgrc[0] : (RECT*)lParam);
				return 0;
			}
			break;
		case WM_NCPAINT:
			if (state->kind == Kind::Edit) {
				paintEditFrame(window);
				return 0;
			}
			break;
		case WM_ERASEBKGND:
			if (state->kind != Kind::Edit && state->kind != Kind::List && state->kind != Kind::Heading) return 1;
			break;
		case WM_NOTIFY:
			// The header's own theme ignores the dark palette, so its text colour is set while it draws.
			if (state->kind == Kind::List) {
				LPNMCUSTOMDRAW draw = (LPNMCUSTOMDRAW)lParam;
				if (draw->hdr.code == NM_CUSTOMDRAW && draw->hdr.hwndFrom == ListView_GetHeader(window)) {
					if (draw->dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
					SetTextColor(draw->hdc, TEXT_PRIMARY);
					return CDRF_DODEFAULT;
				}
			}
			break;
		case WM_PAINT:
			switch (state->kind) {
				case Kind::Check:
				case Kind::Radio:
					paintBuffered(window, [&](HDC dc, RECT rect) { paintToggle(window, dc, rect, state->kind == Kind::Radio, state->labelCommand != 0); });
					return 0;
				case Kind::Push:
					paintBuffered(window, [&](HDC dc, RECT rect) { paintPush(window, dc, rect, *state); });
					return 0;
				case Kind::Card:
					paintBuffered(window, [&](HDC dc, RECT rect) { paintCard(window, dc, rect); });
					return 0;
				case Kind::Combo:
					paintBuffered(window, [&](HDC dc, RECT rect) { paintCombo(window, dc, rect, *state); });
					return 0;
				case Kind::Edit: {
					LRESULT result = DefSubclassProc(window, message, wParam, lParam);
					paintEditPlaceholder(window, *state);
					return result;
				}
				default:
					break;
			}
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			if (state->kind == Kind::Check && !onToggleBox(window, pointOf(lParam))) {
				if (state->labelCommand) {
					state->labelPressed = true;
					SetCapture(window);
				}
				return 0;
			}
			break;
		case WM_LBUTTONUP:
			if (state->labelPressed) {
				state->labelPressed = false;
				ReleaseCapture();
				RECT client;
				GetClientRect(window, &client);
				// Posted so the command runs after this handler, which a modal dialog would otherwise block.
				if (PtInRect(&client, pointOf(lParam))) PostMessage(GetParent(window), WM_COMMAND, MAKEWPARAM(state->labelCommand, BN_CLICKED), (LPARAM)window);
				return 0;
			}
			break;
		case WM_CAPTURECHANGED:
			state->labelPressed = false;
			break;
		case WM_SETCURSOR:
			if (state->labelCommand) {
				POINT cursor;
				GetCursorPos(&cursor);
				ScreenToClient(window, &cursor);
				if (!onToggleBox(window, cursor)) {
					SetCursor(LoadCursor(NULL, IDC_HAND));
					return TRUE;
				}
			}
			break;
		case WM_MOUSEMOVE:
			if (!state->hot && (state->kind == Kind::Push || state->kind == Kind::Combo)) {
				state->hot = true;
				TRACKMOUSEEVENT track = { sizeof(track), TME_LEAVE, window, 0 };
				TrackMouseEvent(&track);
				redraw(window);
			}
			break;
		case WM_MOUSELEAVE:
			state->hot = false;
			redraw(window);
			break;
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_ENABLE:
		case BM_SETCHECK:
		case BM_SETSTATE:
		case WM_SETTEXT: {
			LRESULT result = DefSubclassProc(window, message, wParam, lParam);
			if (state->kind != Kind::Heading) redraw(window);
			return result;
		}
	}
	return DefSubclassProc(window, message, wParam, lParam);
}

State* stateOf(HWND window) {
	DWORD_PTR data = 0;
	return GetWindowSubclass(window, controlProc, SUBCLASS_ID, &data) ? (State*)data : nullptr;
}

void subclass(HWND window, Kind kind) {
	SetWindowSubclass(window, controlProc, SUBCLASS_ID, (DWORD_PTR) new State{ kind, false, NULL, NULL });
}

void darkenEdit(HWND edit) {
	SetWindowLong(edit, GWL_STYLE, GetWindowLong(edit, GWL_STYLE) & ~WS_BORDER);
	SetWindowLong(edit, GWL_EXSTYLE, GetWindowLong(edit, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
	subclass(edit, Kind::Edit);
	SendMessage(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(scaled(edit, EDIT_PADDING), scaled(edit, EDIT_PADDING)));
	SetWindowPos(edit, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void darkenList(HWND list) {
	subclass(list, Kind::List);
	SetWindowTheme(list, L"DarkMode_Explorer", NULL);
	SetWindowTheme(ListView_GetHeader(list), L"DarkMode_ItemsView", NULL);
	ListView_SetBkColor(list, FIELD);
	ListView_SetTextBkColor(list, FIELD);
	ListView_SetTextColor(list, TEXT_PRIMARY);
}

BOOL CALLBACK darkenControl(HWND control, LPARAM) {
	if (hasClass(control, _T("Button"))) {
		switch (GetWindowLong(control, GWL_STYLE) & BS_TYPEMASK) {
			case BS_PUSHBUTTON:
			case BS_DEFPUSHBUTTON:
				subclass(control, Kind::Push);
				break;
			case BS_CHECKBOX:
			case BS_AUTOCHECKBOX:
				subclass(control, Kind::Check);
				break;
			case BS_RADIOBUTTON:
			case BS_AUTORADIOBUTTON:
				subclass(control, Kind::Radio);
				break;
			case BS_GROUPBOX:
				subclass(control, Kind::Card);
				// Dialog controls are created top-down, so a group box starts above the controls it holds and
				// wipes them whenever it repaints on its own. At the bottom and clipped, it cannot.
				SetWindowLong(control, GWL_STYLE, GetWindowLong(control, GWL_STYLE) | WS_CLIPSIBLINGS);
				SetWindowPos(control, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				break;
		}
	} else if (hasClass(control, _T("ComboBox"))) {
		subclass(control, Kind::Combo);
	} else if (hasClass(control, _T("Edit"))) {
		darkenEdit(control);
	} else if (hasClass(control, _T("SysListView32"))) {
		darkenList(control);
	}
	return TRUE;
}

bool highContrast() {
	HIGHCONTRAST contrast = { sizeof(contrast) };
	return SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0) && (contrast.dwFlags & HCF_HIGHCONTRASTON);
}

void apply(HWND dialog) {
	BOOL dark = TRUE;
	COLORREF caption = CARD, text = TEXT_PRIMARY, border = CARD_EDGE;
	DwmSetWindowAttribute(dialog, DWM_USE_DARK_TITLE, &dark, sizeof(dark));
	DwmSetWindowAttribute(dialog, DWM_CAPTION_COLOR, &caption, sizeof(caption));
	DwmSetWindowAttribute(dialog, DWM_TEXT_COLOR, &text, sizeof(text));
	DwmSetWindowAttribute(dialog, DWM_BORDER_COLOR, &border, sizeof(border));
	EnumChildWindows(dialog, darkenControl, 0);
}

} // namespace

bool handleMessage(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam, INT_PTR& result) {
	if (message == WM_INITDIALOG) {
		// High Contrast users choose their own colours, so the dialog keeps the system theme.
		if (!highContrast()) {
			apply(dialog);
			SetProp(dialog, THEMED_PROPERTY, (HANDLE)1);
		}
		return false;
	}
	if (!GetProp(dialog, THEMED_PROPERTY)) return false;
	switch (message) {
		case WM_DESTROY:
			RemoveProp(dialog, THEMED_PROPERTY);
			return false;
		case WM_CTLCOLORDLG:
			result = (INT_PTR)brushOf(BACKGROUND);
			return true;
		case WM_CTLCOLORSTATIC: {
			HWND control = (HWND)lParam;
			bool field = hasClass(control, _T("Edit"));
			COLORREF surface = field ? FIELD : surfaceBehind(control);
			SetBkColor((HDC)wParam, surface);
			SetTextColor((HDC)wParam, IsWindowEnabled(control) ? TEXT_PRIMARY : TEXT_DISABLED);
			result = (INT_PTR)brushOf(surface);
			return true;
		}
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
			SetBkColor((HDC)wParam, FIELD);
			SetTextColor((HDC)wParam, TEXT_PRIMARY);
			result = (INT_PTR)brushOf(FIELD);
			return true;
	}
	return false;
}

void setHeading(HWND label) {
	HFONT base = (HFONT)SendMessage(label, WM_GETFONT, 0, 0);
	LOGFONT font;
	if (!base || !GetObject(base, sizeof(font), &font)) return;
	font.lfWeight = FW_SEMIBOLD;
	HFONT heading = CreateFontIndirect(&font);
	SetWindowSubclass(label, controlProc, SUBCLASS_ID, (DWORD_PTR) new State{ Kind::Heading, false, heading, NULL });
	SendMessage(label, WM_SETFONT, (WPARAM)heading, TRUE);
}

void setLabelCommand(HWND check, UINT command) {
	if (State* state = stateOf(check)) state->labelCommand = command;
}

void setPlaceholder(HWND edit, LPCTSTR text) {
	if (State* state = stateOf(edit)) state->placeholder = text;
}

} // namespace DarkTheme
