#include "pattern.h"

struct Hex_table
{
	BYTE data[256];
};

inline constexpr BYTE hexchar_to_byte(char c)
{
	if (c >= '0' && c <= '9')
		return static_cast<BYTE>(c - '0');
	else if (c >= 'a' && c <= 'f')
		return static_cast<BYTE>(c - 'a' + 10);
	else if (c >= 'A' && c <= 'F')
		return static_cast<BYTE>(c - 'A' + 10);
	else
		return 0xFF;
}

consteval Hex_table make_hex_table()
{
	Hex_table t{};

	for (int i = 0; i < 256; ++i)
		t.data[i] = hexchar_to_byte(static_cast<unsigned char>(i));

	return t;
}

inline constexpr Hex_table lookup_hex = make_hex_table();

inline constexpr BYTE hexchar(char c)
{
	return lookup_hex.data[(unsigned char)c];
}

constexpr BYTE hex_pair(char hi, char lo)
{
	BYTE h = hexchar(hi);
	BYTE l = hexchar(lo);
	//BYTE h = hexchar_to_byte(hi);
	//BYTE l = hexchar_to_byte(lo);
	return (h | l) == 0xFF ? 0xFF : (BYTE)((h << 4) | l);
}

bool get_text_section(HMODULE module, DLL_section* const dll_section) noexcept
{
	if (nullptr == dll_section || !module) {
		return false;
	}

	constexpr const char TEXT_STR[] = ".text";
	constexpr size_t TEXT_LEN = ARRAYSIZE(TEXT_STR) - 1;
	static_assert(
		TEXT_LEN <= IMAGE_SIZEOF_SHORT_NAME,
		"PE section name too long"
		);

	PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
	PIMAGE_NT_HEADERS nt = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<BYTE*>(module) + dos->e_lfanew
		);

	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);
	for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
		if (0 == memcmp(section->Name, TEXT_STR, TEXT_LEN)) {
			dll_section->address = reinterpret_cast<BYTE*>(module) + section->VirtualAddress;
			dll_section->size = section->Misc.VirtualSize;
			return true;
		}
	}
	return false;
}

bool DataCompare(BYTE* pData, BYTE* bSig, char* szMask) noexcept
{
	for (; *szMask; ++szMask, ++pData, ++bSig)
	{
		if (*szMask == 'x' && *pData != *bSig)
			return false;
	}
	return (*szMask) == NULL;
}

BYTE* FindPattern(BYTE* dwAddress, DWORD dwSize, BYTE* pbSig, char* szMask) noexcept
{
	for (DWORD i = NULL; i < dwSize; i++)
	{
		if (DataCompare(dwAddress + i, pbSig, szMask))
			return dwAddress + i;
	}
	return nullptr;
}

// return SIZE_MAX on error.
size_t parse_signaure(
	const char* src,
	size_t src_len,
	BYTE* out_bytes,
	char* out_mask,
	size_t limit
)
{
	size_t i = 0;
	size_t out = 0;

	while (i < src_len)
	{
		// skip whitespace
		char c = src[i];
		if (c == ' ' || c == '\t')
		{
			++i;
			continue;
		}

		// wildcard ??
		if (i + 1 < src_len && src[i] == '?' && src[i + 1] == '?')
		{
			if (out >= limit)
				return SIZE_MAX;

			out_bytes[out] = 0x00;
			out_mask[out] = '?';
			++out;
			i += 2;
			continue;
		}

		// need two chars for hex byte
		if (i + 1 >= src_len)
			return SIZE_MAX;

		const BYTE b = hex_pair(src[i], src[i + 1]);
		if (b == 0xFF)
			return SIZE_MAX;

		if (out >= limit)
			return SIZE_MAX;

		out_bytes[out] = b;
		out_mask[out] = 'x';
		++out;
		i += 2;
	}

	return out; // length of signature/mask
}

size_t parse_hex(
	const char* src,
	size_t src_len,
	BYTE* out_bytes,
	size_t out_cap
)
{
	size_t i = 0;
	size_t out = 0;

	while (i < src_len)
	{
		// skip whitespace
		char c = src[i];
		if (c == ' ' || c == '\t')
		{
			++i;
			continue;
		}

		// need two chars for a byte
		if (i + 1 >= src_len)
			return SIZE_MAX;  // invalid

		const BYTE b = hex_pair(src[i], src[i + 1]);
		if (b == 0xFF)
			return SIZE_MAX;  // invalid hex

		if (out >= out_cap)
			return SIZE_MAX;  // overflow

		out_bytes[out++] = b;
		i += 2;
	}

	return out;
}