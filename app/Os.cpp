#include "Os.h"

#define RUN_ON_STARTUP_KEY_PATH _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define EXE_PATH_SIZE           1024
#define INI_SECTION_MAX_SIZE    32767 // the API limit
#define UNKNOWN_PROGRAM         "UnknownProgram"

static string lastExecutable = APP_NAME_TEXT;
static DWORD lastProcessId = 0;

string Os::toUtf8(const wstring& text) {
	int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), NULL, 0, NULL, NULL);
	string out(size, 0);
	WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), &out[0], size, NULL, NULL);
	return out;
}

wstring Os::toWide(const string& text) {
	int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), NULL, 0);
	wstring out(size, 0);
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), &out[0], size);
	return out;
}

#define SETTINGS_SECTION _T("Settings")

// The settings file sits next to the exe. The UTF-16 BOM makes the INI APIs store Unicode, which exe names may need.
static const wstring& iniPath() {
	static const wstring path = [] {
		wstring exe = Os::getFullPath();
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

void Os::setIntSetting(LPCTSTR key, const int& val) {
	WritePrivateProfileString(SETTINGS_SECTION, key, to_wstring(val).c_str(), iniPath().c_str());
}

int Os::getIntSetting(LPCTSTR key, const int& defaultValue) {
	return (int)GetPrivateProfileInt(SETTINGS_SECTION, key, defaultValue, iniPath().c_str());
}

vector<pair<wstring, wstring>> Os::readSection(LPCTSTR section) {
	vector<TCHAR> buffer(INI_SECTION_MAX_SIZE);
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

void Os::writeSection(LPCTSTR section, const vector<pair<wstring, wstring>>& entries) {
	WritePrivateProfileString(section, NULL, NULL, iniPath().c_str());
	for (const auto& entry : entries) {
		WritePrivateProfileString(section, entry.first.c_str(), entry.second.c_str(), iniPath().c_str());
	}
}

static void deleteStartupValue() {
	HKEY key;
	RegOpenKeyEx(HKEY_CURRENT_USER, RUN_ON_STARTUP_KEY_PATH, 0, KEY_ALL_ACCESS, &key);
	RegDeleteValue(key, APP_NAME);
	RegCloseKey(key);
}

void Os::registerRunOnStartup(const int& val) {
	if (!val) {
		deleteStartupValue();
		WinExec("schtasks /delete /tn " APP_NAME_TEXT " /f", SW_HIDE);
	} else if (vRunAsAdmin) {
		string path = toUtf8(getFullPath());
		char buff[MAX_PATH + 100];
		sprintf(buff, "schtasks /create /sc onlogon /tn %s /rl highest /tr \"%s\" /f", APP_NAME_TEXT, path.c_str());
		WinExec(buff, SW_HIDE);
	} else {
		HKEY key;
		RegOpenKeyEx(HKEY_CURRENT_USER, RUN_ON_STARTUP_KEY_PATH, 0, KEY_ALL_ACCESS, &key);
		wstring path = L"\"" + getFullPath() + L"\""; // quoted so a folder name with spaces still launches
		RegSetValueEx(key, APP_NAME, 0, REG_SZ, (LPBYTE)path.c_str(), ((DWORD)path.size() + 1) * sizeof(TCHAR));
		RegCloseKey(key);
	}
}

string Os::getExecutableName(const wstring& exePath) {
	size_t slash = exePath.find_last_of(L"\\/");
	return toUtf8(exePath.substr(slash == wstring::npos ? 0 : slash + 1));
}

// Taskbar, desktop and this app's own windows keep the previous app's name,
// so focusing them never switches a rule.
const string& Os::getForegroundExecutableName() {
	static const string selfExe = getExecutableName(getFullPath());
	DWORD processId;
	GetWindowThreadProcessId(GetForegroundWindow(), &processId);
	if (processId == lastProcessId) {
		return lastExecutable;
	}
	lastProcessId = processId;
	TCHAR path[EXE_PATH_SIZE] = { 0 };
	HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
	if (process) {
		GetProcessImageFileName(process, path, EXE_PATH_SIZE);
		CloseHandle(process);
	}
	if (path[0] == 0) {
		lastProcessId = 0;
		static const string unknown = UNKNOWN_PROGRAM;
		return unknown;
	}
	const TCHAR* name = _tcsrchr(path, '\\') + 1;
	if (wcsicmp(name, L"explorer.exe") == 0 || getExecutableName(name) == selfExe) {
		return lastExecutable;
	}
	lastExecutable = toUtf8(name);
	return lastExecutable;
}

const string& Os::getLastExecutableName() {
	return lastExecutable;
}

wstring Os::getFullPath() {
	TCHAR path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);
	return wstring(path);
}

void Os::setClipboardText(LPCTSTR data, const int& len, const int& type) {
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(WCHAR));
	memcpy(GlobalLock(hMem), data, len * sizeof(WCHAR));
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(type, hMem);
	CloseClipboard();
}
