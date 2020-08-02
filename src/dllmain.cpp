// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

extern bool g_skip_wpad;
extern std::wofstream logging;
extern _getaddrinfo getaddrinfo_orig;

static const char* configFile = "./config.ini";

void init_log (const char* logFile)
{
	if (1 == GetPrivateProfileInt ("Config", "Log", 0, configFile)) {		
		logging.open (logFile, std::ios::out | std::ios::app);
		logging << "BlockTheSpot - Build date: " << __TIMESTAMP__ << std::endl;
	}
}

BOOL APIENTRY DllMain (HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved
)
{
	DisableThreadLibraryCalls (hModule);
	std::string procname = GetCommandLine ();
	// only Spotify process - this help avoid false positive
	if (std::string::npos != procname.find ("Spotify.exe")) {
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
			if (std::string::npos == procname.find ("--type=")) {
				// block ads request - main process
				CreateThread (NULL, NULL, KillBanner, NULL, 0, NULL);
				init_log ("main_log.txt");
			}
			else if (std::string::npos != procname.find ("--type=utility")) {
				// block ads banner by hostname - utility process
				init_log ("utility_log.txt");
				getaddrinfo_orig = (_getaddrinfo)GetProcAddress (GetModuleHandle ("ws2_32.dll"), "getaddrinfo");
				if (getaddrinfo_orig) {
					Mhook_SetHook ((PVOID*)&getaddrinfo_orig, getaddrinfo_hook);
					if (logging.is_open ())
						logging << "Mhook_SetHook - getaddrinfo success!" << std::endl;
				}
				else {
					if (logging.is_open ())
						logging << "Mhook_SetHook - getaddrinfo failed!" << std::endl;
				}
				if (1 == GetPrivateProfileInt ("Config", "Skip_wpad", 0, configFile))
					g_skip_wpad = true;
			}
			break;
		case DLL_PROCESS_DETACH:
			if (logging.is_open ()) {
				logging.flush ();
				logging.close ();
			}
			break;
		}
	}
	return TRUE;
}

