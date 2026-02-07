#pragma once
#define _CRT_SECURE_CPP_NOTHROW
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <winternl.h>
#include <cstdint>
#include <cstdio>

#define USE_APC
//#define USE_THREAD

#if defined(USE_THREAD) && defined(USE_APC)
#error "Cannot define both USE_THREAD and USE_APC"
#endif

inline constexpr auto ORIGINAL_CHROME_ELF_DLL = L"chrome_elf_required.dll";
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

inline ImageDirectoryEntryToDataEx_t ImageDirectoryEntryToDataEx = nullptr;

constexpr size_t SHARED_BUFFER_SIZE = 1024; // increase if need.
inline char shared_buffer[SHARED_BUFFER_SIZE];

constexpr size_t MAX_CEF_BLOCK_LIST = 5;
constexpr size_t MAX_CEF_BUFFER_MODIFY_LIST = 5;
constexpr size_t MAX_URL_LEN = 50;

inline size_t CEF_REQUEST_GET_URL_OFFSET = 0x30;
inline size_t CEF_ZIP_READER_GET_FILE_NAME_OFFSET = 0x48;
inline size_t CEF_ZIP_READER_GET_READ_FILE_OFFSET = 0x70;

VOID CALLBACK bts_main(ULONG_PTR param);
bool remove_debug_log() noexcept;
