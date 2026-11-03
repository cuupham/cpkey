#pragma once
#include "common.h"
#include "MainControlDialog.h"
#include "AppRulesDialog.h"

class App {
  private:
	HINSTANCE hInstance = NULL;
	BaseDialog *mainDialog = NULL, *rulesDialog = NULL;

  private:
	bool isDialogMsg(MSG& msg) const;

  public:
	App();
	static App* getInstance();
	int run(HINSTANCE instance);
	void closeDialog(BaseDialog* dialog);

  public:
	void refreshUi();
	void onDefaultConfig();

	void onToggleCheckSpelling();
	void onInputType(const int& type);

	void onAppRules();
	void onControlPanel();
	void onExit();
};
