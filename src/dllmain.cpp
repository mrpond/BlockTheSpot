#include "config_manager.h"
#include <windows.h>
#include <string>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	std::wstring_view procname = GetCommandLineW();
	if (std::wstring_view::npos != procname.find(L"Spotify.exe")) {
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
			if (std::wstring_view::npos == procname.find(L"--type=")) {
            	DisableThreadLibraryCalls(hModule);
				ConfigManager::Initialize();
            }
			break;
		}
	}
	return TRUE;
}