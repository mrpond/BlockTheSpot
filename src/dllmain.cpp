#include "pch.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	DisableThreadLibraryCalls(hModule);
	std::wstring_view procname = GetCommandLineW();
	if (std::wstring_view::npos != procname.find(L"Spotify.exe")) {
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
			if (std::wstring_view::npos == procname.find(L"--type=")) {
				HANDLE hThread = nullptr;
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
					hThread = CreateThread(NULL, 0, BlockAds, NULL, 0, NULL);
					if (hThread != nullptr) {
						CloseHandle(hThread);
					}
				}

				if (SettingsManager::m_config.at(L"Block_Banner")) {
					hThread = CreateThread(NULL, 0, BlockBanner, NULL, 0, NULL);
					if (hThread != nullptr) {
						CloseHandle(hThread);
					}
				}

				if (SettingsManager::m_config.at(L"Enable_Developer")) {
					hThread = CreateThread(NULL, 0, EnableDeveloper, NULL, 0, NULL);
					if (hThread != nullptr) {
						CloseHandle(hThread);
					}
				}
			}
			break;
		}
	}
	return TRUE;
}