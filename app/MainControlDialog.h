#pragma once
#include "BaseDialog.h"

class MainControlDialog : public BaseDialog {
  private:
	HWND comboBoxInputType;
	HWND textSwitchKey;
	bool advancedOpen = false;

  private:
	void initDialog();
	void setAdvancedOpen(bool open);
	void updateRulesButton();
	void onCheckboxClicked(int controlId);
	void onSwitchKeyPressed(const UINT16& keyCode);
	void setSwitchKeyText(const UINT16& keyCode);
	void requestRestartAsAdmin();

  protected:
	INT_PTR eventProc(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

  public:
	MainControlDialog(const HINSTANCE& instance, const int& resourceId);
	void refresh() override;
};
