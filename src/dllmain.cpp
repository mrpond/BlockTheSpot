// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "Modify.h"

BOOL APIENTRY DllMain (HMODULE hModule,
					   DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	std::string_view procname = GetCommandLine ();
	// only Spotify process - this help avoid false positive
	if (std::string_view::npos != procname.find ("Spotify.exe")) {
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
			if (std::string_view::npos == procname.find ("--type=")) {
				// block ads request - main process
				CreateThread (NULL, NULL, KillBanner, NULL, 0, NULL);
			}
			
			break;
		}
	}
	return TRUE;
}

