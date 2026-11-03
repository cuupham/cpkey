#pragma once
#include "stdafx.h"

class BaseDialog {
  protected:
	UINT resId;
	HINSTANCE hInstance;
	HWND hDlg;

  protected:
	virtual INT_PTR eventProc(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
	void setDialogIcon();

  public:
	BaseDialog(const HINSTANCE& instance, const int& resourceId);
	virtual ~BaseDialog() {}
	void show();
	void bringOnTop();
	virtual void fillData() = 0;

	HWND getHwnd();
	void closeDialog();
	static INT_PTR CALLBACK dialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
