#include "SystemHelper.h"

static HKEY hKey;
static LPCTSTR _runOnStartupKeyPath = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run");

static DWORD _cacheProcessId = 0, _tempProcessId = 0;
static HWND _tempWnd;
static TCHAR _exePath[1024] = { 0 };
static LPCTSTR _exeName = _exePath;
static HANDLE _proc;
static string _exeNameUtf8 = APP_NAME_TEXT;
static string _unknownProgram = "UnknownProgram";

string SystemHelper::toUtf8(const wstring& text) {
	int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), NULL, 0, NULL, NULL);
	string out(size, 0);
	WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), &out[0], size, NULL, NULL);
	return out;
}

wstring SystemHelper::toWide(const string& text) {
	int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), NULL, 0);
	wstring out(size, 0);
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), &out[0], size);
	return out;
}

#define SETTINGS_SECTION _T("Settings")

// Everything is stored in an INI file next to the exe, so deleting the folder removes all traces.
// The UTF-16 BOM makes the INI APIs store Unicode, which exe names may need.
static const wstring& iniPath() {
	static const wstring path = [] {
		wstring exe = SystemHelper::getFullPath();
		wstring ini = exe.substr(0, exe.find_last_of(L'.')) + L".ini";
		HANDLE file = CreateFile(ini.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file != INVALID_HANDLE_VALUE) {
			const WORD bom = 0xFEFF;
			DWORD written;
			WriteFile(file, &bom, sizeof(bom), &written, NULL);
			CloseHandle(file);
		}
		return ini;
	}();
	return path;
}

void SystemHelper::setIntSetting(LPCTSTR key, const int& val) {
	WritePrivateProfileString(SETTINGS_SECTION, key, to_wstring(val).c_str(), iniPath().c_str());
}

int SystemHelper::getIntSetting(LPCTSTR key, const int& defaultValue) {
	return (int)GetPrivateProfileInt(SETTINGS_SECTION, key, defaultValue, iniPath().c_str());
}

vector<pair<wstring, wstring>> SystemHelper::readSection(LPCTSTR section) {
	vector<TCHAR> buffer(32767);
	DWORD size = GetPrivateProfileSection(section, buffer.data(), (DWORD)buffer.size(), iniPath().c_str());
	vector<pair<wstring, wstring>> entries;
	for (const TCHAR* line = buffer.data(); line < buffer.data() + size && *line; line += lstrlen(line) + 1) {
		wstring text = line;
		size_t equals = text.find_last_of(L'=');
		if (equals != wstring::npos) {
			entries.emplace_back(text.substr(0, equals), text.substr(equals + 1));
		}
	}
	return entries;
}

void SystemHelper::writeSection(LPCTSTR section, const vector<pair<wstring, wstring>>& entries) {
	WritePrivateProfileString(section, NULL, NULL, iniPath().c_str());
	for (const auto& entry : entries) {
		WritePrivateProfileString(section, entry.first.c_str(), entry.second.c_str(), iniPath().c_str());
	}
}

void SystemHelper::registerRunOnStartup(const int& val) {
	if (val) {
		if (vRunAsAdmin) {
			string path = toUtf8(getFullPath());
			char buff[MAX_PATH + 100];
			sprintf(buff, "schtasks /create /sc onlogon /tn %s /rl highest /tr \"%s\" /f", APP_NAME_TEXT, path.c_str());
			WinExec(buff, SW_HIDE);
		} else {
			RegOpenKeyEx(HKEY_CURRENT_USER, _runOnStartupKeyPath, NULL, KEY_ALL_ACCESS, &hKey);
			wstring path = L"\"" + getFullPath() + L"\""; // quoted so a folder name with spaces still launches
			RegSetValueEx(hKey, APP_NAME, 0, REG_SZ, (LPBYTE)path.c_str(), ((DWORD)path.size() + 1) * sizeof(TCHAR));
			RegCloseKey(hKey);
		}
	} else {
		RegOpenKeyEx(HKEY_CURRENT_USER, _runOnStartupKeyPath, NULL, KEY_ALL_ACCESS, &hKey);
		RegDeleteValue(hKey, APP_NAME);
		RegCloseKey(hKey);
		WinExec("schtasks /delete /tn " APP_NAME_TEXT " /f", SW_HIDE);
	}
}

string SystemHelper::getExecuteName(const wstring& exePath) {
	size_t slash = exePath.find_last_of(L"\\/");
	return toUtf8(exePath.substr(slash == wstring::npos ? 0 : slash + 1));
}

// Taskbar, desktop and this app's own windows keep the previous app's name,
// so focusing them never switches a rule.
string& SystemHelper::getFrontMostAppExecuteName() {
	static const string selfExe = getExecuteName(getFullPath());
	_tempWnd = GetForegroundWindow();
	GetWindowThreadProcessId(_tempWnd, &_tempProcessId);
	if (_tempProcessId == _cacheProcessId) {
		return _exeNameUtf8;
	}
	_cacheProcessId = _tempProcessId;
	_exePath[0] = 0;
	_proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, _tempProcessId);
	if (_proc) {
		GetProcessImageFileName(_proc, _exePath, 1024);
		CloseHandle(_proc);
	}
	if (_exePath[0] == 0) {
		_cacheProcessId = 0;
		return _unknownProgram;
	}
	_exeName = _tcsrchr(_exePath, '\\') + 1;
	if (wcsicmp(_exeName, L"explorer.exe") == 0 || getExecuteName(_exeName) == selfExe) {
		return _exeNameUtf8;
	}
	_exeNameUtf8 = toUtf8(_exeName);
	return _exeNameUtf8;
}

string& SystemHelper::getLastAppExecuteName() {
	return _exeNameUtf8;
}

wstring SystemHelper::getFullPath() {
	TCHAR path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);
	return wstring(path);
}

void SystemHelper::setClipboardText(LPCTSTR data, const int& len, const int& type) {
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(WCHAR));
	memcpy(GlobalLock(hMem), data, len * sizeof(WCHAR));
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(type, hMem);
	CloseClipboard();
}
