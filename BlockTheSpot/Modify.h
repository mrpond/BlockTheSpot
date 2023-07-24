#pragma once

DWORD WINAPI EnableDeveloper(LPVOID lpParam);
DWORD WINAPI BlockAds(LPVOID lpParam);
DWORD WINAPI BlockBanner(LPVOID lpParam);

extern "C" void WINAPI get_file_name();
extern "C" void WINAPI modify_source();

extern "C" PatternScanner::ModuleInfo ZipScan;

extern "C" std::uint64_t file_name_rcx;
extern "C" std::uint64_t ret_addr_file_name;
extern "C" std::uint64_t ret_addr_file_source;
