#pragma once
#include "common.h"

class Os {
  public:
	static void setIntSetting(LPCTSTR key, const int& val);
	static int getIntSetting(LPCTSTR key, const int& defaultValue);

	static vector<pair<wstring, wstring>> readSection(LPCTSTR section);
	static void writeSection(LPCTSTR section, const vector<pair<wstring, wstring>>& entries);

	static bool isRunOnStartupRegistered();
	static void registerRunOnStartup(const int& val);

	static const string& getForegroundExecutableName();
	static const string& getLastExecutableName();
	static string getExecutableName(const wstring& exePath);

	static string toUtf8(const wstring& text);
	static wstring toWide(const string& text);

	static wstring getFullPath();

	static void setClipboardText(LPCTSTR data, const int& len, const int& type);
};
