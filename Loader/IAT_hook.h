#pragma once
#include "loader.h"

using GetProcAddress_t = FARPROC(WINAPI*)(HMODULE, LPCSTR);
inline GetProcAddress_t GetProcAddress_orig = nullptr;

bool process_IAT_hook_GetProcAddress(HMODULE module) noexcept;
//bool IAT_unhook_GetProcAddress() noexcept;
