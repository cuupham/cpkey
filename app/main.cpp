#include "common.h"
#include "App.h"
#include <shlobj.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
	// Relaunch elevated so it can type into elevated windows.
	APP_GET_DATA(vRunAsAdmin, 0);
	if (vRunAsAdmin && !IsUserAnAdmin()) {
		ShellExecute(0, L"runas", Os::getFullPath().c_str(), 0, 0, SW_SHOWNORMAL);
		return 1;
	}
	App app;
	return app.run(hInstance);
}
