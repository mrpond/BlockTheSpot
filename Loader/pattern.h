#pragma once
#include "loader.h"

struct DLL_section
{
	DWORD size;
	BYTE* address;
};

struct Modify
{
	uint8_t signature[SHARED_BUFFER_SIZE];
	char mask[SHARED_BUFFER_SIZE];
	UINT offset;
	uint8_t value[SHARED_BUFFER_SIZE];
	size_t patch_size;
};

// https://www.unknowncheats.me/forum/1064672-post23.html
bool DataCompare(BYTE* pData, BYTE* bSig, char* szMask) noexcept;
BYTE* FindPattern(BYTE* dwAddress, DWORD dwSize, BYTE* pbSig, char* szMask) noexcept;

bool get_text_section(HMODULE module, DLL_section* const dll_section) noexcept;

size_t parse_signaure(
	const char* src,
	size_t src_len,
	BYTE* out_bytes,
	char* out_mask,
	size_t out_cap
) noexcept;

size_t parse_hex(
	const char* src,
	size_t src_len,
	BYTE* out_bytes,
	size_t out_cap
) noexcept;
