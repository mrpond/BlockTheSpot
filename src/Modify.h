#pragma once

bool DataCompare (BYTE* pData, BYTE* bSig, char* szMask);

BYTE* FindPattern (BYTE* dwAddress, DWORD dwSize, BYTE* pbSig, char* szMask);

DWORD WINAPI KillBanner (LPVOID);