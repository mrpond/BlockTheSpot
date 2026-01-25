#pragma once
#include "loader.h"

#include <WinTrust.h>
#pragma comment(lib, "Wintrust.lib")

using WinVerifyTrust_t = LONG(WINAPI*)(HWND, GUID*, LPVOID);
LONG WINAPI WinVerifyTrust_hook(HWND hwnd, GUID* pgActionID, LPVOID pWVTData);
//static WinVerifyTrust_t WinVerifyTrust_orig = nullptr;
