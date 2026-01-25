#include "pch.h"

extern "C"
LPVOID WINAPI load_api(const char* api_name)
{
    static HMODULE hModule = GetModuleHandleW(L"dpapi.dll");
    if (!hModule) {
        hModule = LoadLibraryW(L"dpapi.dll");
        if (!hModule) {
            return nullptr;
        }
    }
    return GetProcAddress(hModule, api_name);
}
