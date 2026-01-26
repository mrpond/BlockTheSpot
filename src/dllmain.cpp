#include "pch.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	if (DLL_PROCESS_ATTACH == ul_reason_for_call) {
		DisableThreadLibraryCalls(hModule);
		auto cmd = GetCommandLineW();
		if (NULL != wcsstr(cmd, L"Spotify.exe")) {
			if (NULL == wcsstr(cmd, L"--type=") &&
				NULL == wcsstr(cmd, L"--url="))
			{
				QueueUserWorkItem(
					[](LPVOID) -> DWORD {

#ifndef NDEBUG
						_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
						if (AllocConsole()) {
							if (_wfreopen(L"CONIN$", L"r", stdin) == nullptr)
								MessageBoxW(0, L"Failed to redirect standard input", L"Error", 0);
							if (_wfreopen(L"CONOUT$", L"w", stdout) == nullptr)
								MessageBoxW(0, L"Failed to redirect standard output", L"Error", 0);
							if (_wfreopen(L"CONOUT$", L"w", stderr) == nullptr)
								MessageBoxW(0, L"Failed to redirect standard error", L"Error", 0);

							SetConsoleOutputCP(CP_UTF8);
							SetConsoleCP(CP_UTF8);
						}

						hThread = CreateThread(NULL, 0, Debug, NULL, 0, NULL);
						if (hThread != nullptr) {
							CloseHandle(hThread);
						}
#endif
						SettingsManager::Init();
						if (SettingsManager::m_config.at(L"Block_Ads")) {
							BlockAds(nullptr);
						}
						if (SettingsManager::m_config.at(L"Block_Banner")) {
							BlockBanner(nullptr);
						}
						return 0;
					},
					nullptr,
					WT_EXECUTEDEFAULT
				);
			}
		}
	}
	return TRUE;
}