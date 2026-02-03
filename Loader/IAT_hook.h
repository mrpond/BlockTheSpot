#pragma once
#include "loader.h"

using GetProcAddress_t = FARPROC(WINAPI*)(HMODULE, LPCSTR);
inline GetProcAddress_t GetProcAddress_orig = nullptr;

bool IAT_hook_GetProcAddress(HMODULE module = GetModuleHandleW(nullptr)) noexcept;
//bool IAT_unhook_GetProcAddress() noexcept;
