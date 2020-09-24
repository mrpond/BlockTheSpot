#include "Modify.h"
#include "Logger.h"

extern Logger g_Logger;

// https://www.unknowncheats.me/forum/1064672-post23.html
bool DataCompare (BYTE* pData, BYTE* bSig, char* szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bSig)
	{
		if (*szMask == 'x' && *pData != *bSig)
			return false;
	}
	return (*szMask) == NULL;
}

BYTE* FindPattern (BYTE* dwAddress, DWORD dwSize, BYTE* pbSig, char* szMask)
{
	DWORD length = strlen (szMask);
	for (DWORD i = NULL; i < dwSize - length; i++)
	{
		__try
		{
			if (DataCompare (dwAddress + i, pbSig, szMask))
				return dwAddress + i;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return nullptr;
		}
	}
	return 0;
}

DWORD WINAPI KillBanner (LPVOID)
{
	HMODULE hModule = GetModuleHandle (NULL);
	MODULEINFO mInfo = { 0 };
	if (GetModuleInformation (GetCurrentProcess (), hModule, &mInfo, sizeof (MODULEINFO))) {

		LPVOID skipPod = FindPattern ((uint8_t*)hModule, mInfo.SizeOfImage, (BYTE*)"\x20\x00\x70\x6F\x64\x00", "xxxxxx");
		
		if (skipPod)
		{
			DWORD oldProtect;
			VirtualProtect ((char*)skipPod + 3, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
			memset ((char*)skipPod + 3, 0x65, 1);
			VirtualProtect ((char*)skipPod + 3, 1, oldProtect, &oldProtect);
			g_Logger.Log ("main process - patch success!");
		}
		else {
			g_Logger.Log ("main process - patch failed!");
		}

	}
	return 0;
}
