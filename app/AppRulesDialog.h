#pragma once
#include "BaseDialog.h"

class AppRulesDialog : public BaseDialog {
  private:
	HWND hList;

  private:
	void initDialog();
	void onAdd();
	void onToggle();
	void onDelete();
	int selectedIndex();
	string appAt(int index);

  protected:
	INT_PTR eventProc(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

  public:
	AppRulesDialog(const HINSTANCE& instance, const int& resourceId);
	void fillData() override;
};
