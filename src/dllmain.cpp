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
						SettingsManager::Init();
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