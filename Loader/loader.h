#pragma once
#define _CRT_SECURE_CPP_NOTHROW
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <winternl.h>
#include <cstdint>
#include <cstdio>

inline constexpr auto ORIGINAL_CHROME_ELF_DLL = L"chrome_elf_bak.dll";
inline constexpr auto CONFIG_FILEW = L".\\config.ini";
inline constexpr auto CONFIG_FILEA = ".\\config.ini";
inline constexpr auto LOG_FILEW = L".\\fucking.log";

using ImageDirectoryEntryToDataEx_t = PVOID(WINAPI*)(
	PVOID Base,
	BOOLEAN MappedAsImage,
	USHORT DirectoryEntry,
	PULONG Size,
	PIMAGE_SECTION_HEADER* FoundHeader
	);

inline HMODULE libcef_dll_handle = nullptr;
inline ImageDirectoryEntryToDataEx_t ImageDirectoryEntryToDataEx = nullptr;

constexpr size_t SHARED_BUFFER_SIZE = 128; // increase if need.
inline char shared_buffer[SHARED_BUFFER_SIZE];

constexpr size_t MAX_CEF_BLOCK_LIST = 5;
constexpr size_t MAX_CEF_BUFFER_MODIFY_LIST = 5;
constexpr size_t MAX_URL_LEN = 50;

bool remove_unused_dll() noexcept;
const wchar_t* filename_from_path(const wchar_t* path) noexcept;
VOID CALLBACK bts_main(ULONG_PTR param);