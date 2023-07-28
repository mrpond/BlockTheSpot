#pragma once
DWORD WINAPI EnableDeveloper(LPVOID lpParam);
DWORD WINAPI BlockAds(LPVOID lpParam);
DWORD WINAPI BlockBanner(LPVOID lpParam);

#ifdef _WIN64
extern "C" void WINAPI get_file_name();
extern "C" void WINAPI modify_source();

extern "C" PatternScanner::ModuleInfo ZipScan;
extern "C" std::uintptr_t file_name_pointer;
extern "C" std::uintptr_t ret_addr_file_name;
extern "C" std::uintptr_t ret_addr_file_source;
#endif
