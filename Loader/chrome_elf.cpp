#include "loader.h"

extern "C"
LPVOID WINAPI load_api(const char* api_name)
{
    static HMODULE hModule = GetModuleHandleW(ORIGINAL_CHROME_ELF_DLL);
    if (!hModule) {
        hModule = LoadLibraryW(ORIGINAL_CHROME_ELF_DLL);
        if (!hModule) {
            return nullptr;
        }
    }
    return GetProcAddress(hModule, api_name);
}
