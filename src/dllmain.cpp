#include "pch.h"

bool block_ads = true;
bool block_banner = true;
bool enable_developer = true;
bool enable_log = false;

void SyncConfigFile() {
	std::wstring ini_path = L".\\config.ini";
	std::map<std::wstring, bool*> config = {
		{L"Block_Ads", &block_ads},
		{L"Block_Banner", &block_banner},
		{L"Enable_Developer", &enable_developer},
		{L"Enable_Log", &enable_log},
	};

	for (const auto& [key, bool_ptr] : config) {
		std::wstring current_value = Utils::ReadIniFile(ini_path, L"Config", key);
		if (current_value.empty() || current_value != L"1" && current_value != L"0") {
			Utils::WriteIniFile(ini_path, L"Config", key, *bool_ptr ? L"1" : L"0");
		}
		else {
			*bool_ptr = (current_value == L"1");
		}
	}

	PrintStatus(block_ads, L"Block ADS");
	PrintStatus(block_banner, L"Block Banner");
	PrintStatus(enable_developer, L"Enable Developer");
	PrintStatus(enable_log, L"Enable Log");
}

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

				SyncConfigFile();
				Logger::Init(L"blockthespot.log", enable_log);

				if (block_ads) {
					hThread = CreateThread(NULL, 0, BlockAds, NULL, 0, NULL);
					if (hThread != nullptr) {
						CloseHandle(hThread);
					}
				}

				if (block_banner) {
					hThread = CreateThread(NULL, 0, BlockBanner, NULL, 0, NULL);
					if (hThread != nullptr) {
						CloseHandle(hThread);
					}
				}

				if (enable_developer) {
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