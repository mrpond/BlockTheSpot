#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <winternl.h>
#include <cstdint>

inline constexpr auto ORIGINAL_CHROME_ELF_DLL = L"chrome_elf_bak.dll";
inline constexpr auto CONFIG_FILEW = L".\\config.ini";
inline constexpr auto CONFIG_FILEA = ".\\config.ini";
inline constexpr auto LOG_FILEW = L".\\fucking.log";

constexpr size_t SHARED_BUFFER_SIZE = 32; // increase if need.
inline char shared_buffer[SHARED_BUFFER_SIZE];

bool remove_unused_dll() noexcept;
const wchar_t* filename_from_path(const wchar_t* path) noexcept;
VOID bts_main(ULONG_PTR param);