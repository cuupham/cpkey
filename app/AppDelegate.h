#pragma once
#include "stdafx.h"
#include "MainControlDialog.h"
#include "AppRulesDialog.h"

class AppDelegate {
private:
	HINSTANCE hInstance;
	BaseDialog* mainDialog = NULL, *rulesDialog = NULL;
private:
	bool isDialogMsg(MSG &msg) const;
public:
	AppDelegate();
	static AppDelegate* getInstance();
	int run(HINSTANCE instance);
	void createMainDialog();
	void closeDialog(BaseDialog* dialog);
public:
	void refreshUi();
	void onDefaultConfig();

	void onToggleVietnamese();
	void onToggleCheckSpelling();
	void onInputType(const int& type);

	void onAppRules();
	void onControlPanel();
	void onExit();
};
