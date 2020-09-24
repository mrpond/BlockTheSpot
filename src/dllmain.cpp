// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "Config.h"
#include "hosts.h"
#include "Logger.h"
#include "Modify.h"

extern _getaddrinfo getaddrinfo_orig;
Logger g_Logger;
Config g_Config;



BOOL APIENTRY DllMain (HMODULE hModule,
					   DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	DisableThreadLibraryCalls (hModule);
	std::string procname = GetCommandLine ();
	// only Spotify process - this help avoid false positive
	if (std::string::npos != procname.find ("Spotify.exe")) {
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
			if (std::string::npos == procname.find ("--type=") && false == g_Config.getConfig("Block_BannerOnly")) {
				// block ads request - main process
				CreateThread (NULL, NULL, KillBanner, NULL, 0, NULL);
				g_Logger.setLogfile ("main_log.txt");
			}
			else if (std::string::npos != procname.find ("--type=utility")) {
				// block ads banner by hostname - utility process
				g_Logger.setLogfile ("utility_log.txt");
				getaddrinfo_orig = getaddrinfo;
				if (getaddrinfo_orig) {
					Mhook_SetHook ((PVOID*)&getaddrinfo_orig, getaddrinfo_hook);
					g_Logger.Log ("Mhook_SetHook - getaddrinfo success!");
				}
				else {
					g_Logger.Log ("Mhook_SetHook - getaddrinfo failed!");
				}
			}
			break;
		}
	}
	return TRUE;
}

