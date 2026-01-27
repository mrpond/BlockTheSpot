#include "cef_url_hook.h"
#include "loader.h"
#include "funct_pointer.h"
#include "log_thread.h"

static inline size_t cef_block_count = 0;

static inline bool is_blocked(const char* in_url) noexcept {
	for (size_t i = 0; i < cef_block_count; ++i) {
		const char* block_url = cef_block_list[i];

		if (strstr(in_url, block_url)) {
			return true;
		}
	}
	return false;
}

#ifdef USE_LIBCEF
void* cef_urlrequest_create_hook(struct _cef_request_t* request, void* client, void* request_context)
#else
void* cef_urlrequest_create_hook(void* request, void* client, void* request_context)
#endif
{
#ifdef USE_LIBCEF
	cef_string_utf16_t* url_utf16 = request->get_url(request);
	const wchar_t* url = url_utf16->str;
#else
	using cef_request_get_url_t = void* (__stdcall*)(void*);
	cef_request_get_url_t get_url = get_funct_t<cef_request_get_url_t>(request, 48);
	const auto url_utf16 = get_url(request);
	const wchar_t* url = *reinterpret_cast<wchar_t**>(url_utf16);
#endif
	
	char log_buf[256];
	const auto len = WideCharToMultiByte(CP_ACP, 0, url, -1, shared_buffer, SHARED_BUFFER_SIZE, NULL, NULL);
	cef_string_userfree_utf16_free_orig((void*)url_utf16);

	if (0 == len) {
		return cef_urlrequest_create_orig(request, client, request_context);
	}

	const bool blocked = is_blocked(shared_buffer);

	_snprintf_s(
		log_buf,
		sizeof(log_buf),
		_TRUNCATE,
		"%s:%s",
		blocked ? "block" : "allow",
		shared_buffer
	);
	log_debug(log_buf);

	return blocked ? nullptr : cef_urlrequest_create_orig(request, client, request_context);
}

static inline bool is_cef_url_hook() noexcept
{
	auto result = GetPrivateProfileIntA(
		"Config",
		"URL_block",
		0,
		CONFIG_FILEA
	);
	return 0 != result;
}

// https://www.ired.team/offensive-security/code-injection-process-injection/import-adress-table-iat-hooking
static inline bool EAT_hook_cef_url(HMODULE libcef_dll_handle) noexcept
{
	HMODULE module = libcef_dll_handle;
	if (!module) return false;

	PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
	PIMAGE_NT_HEADERS nt = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<BYTE*>(module) + dos->e_lfanew
		);

	IMAGE_DATA_DIRECTORY& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (!dir.VirtualAddress) return false;

	PIMAGE_EXPORT_DIRECTORY export_dir = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
		reinterpret_cast<BYTE*>(module) + dir.VirtualAddress
		);
	auto names = reinterpret_cast<DWORD*>(
		reinterpret_cast<BYTE*>(module) + export_dir->AddressOfNames
		);
	auto ordinal = reinterpret_cast<WORD*>(reinterpret_cast<BYTE*>(module) +
		export_dir->AddressOfNameOrdinals
		);
	auto function = reinterpret_cast<DWORD*>(
		reinterpret_cast<BYTE*>(module) + export_dir->AddressOfFunctions
		);

	for (DWORD i =0; i < export_dir->NumberOfNames; ++i) {
		const char* name = reinterpret_cast<const char*>(
			reinterpret_cast<BYTE*>(module) + names[i]
			);


		if (0 == lstrcmpiA(name, "cef_urlrequest_create")) {
			DWORD* func_rva = &function[ordinal[i]];
			DWORD rva = *func_rva;

			cef_urlrequest_create_orig = 
				reinterpret_cast<cef_urlrequest_create_t>(
					reinterpret_cast<BYTE*>(module) + rva);

			DWORD oldProtect;
			VirtualProtect(func_rva, sizeof(DWORD), PAGE_READWRITE, &oldProtect);
			*func_rva = static_cast<DWORD>(
				reinterpret_cast<BYTE*>(cef_urlrequest_create_hook) -
				reinterpret_cast<BYTE*>(module));
			VirtualProtect(func_rva, sizeof(DWORD), oldProtect, &oldProtect);
		}
	}
	return true;
}

static inline void do_hook_cef_url(HMODULE libcef_dll_handle) noexcept
{
	EAT_hook_cef_url(libcef_dll_handle);
	cef_string_userfree_utf16_free_orig = reinterpret_cast<cef_string_userfree_utf16_free_t>(
		GetProcAddress(
			libcef_dll_handle,
			"cef_string_userfree_utf16_free"
		));
	cef_block_count = GetPrivateProfileIntA(
		"Config",
		"URL_block",
		0,
		CONFIG_FILEA
	);
	for (size_t i = 0; i < cef_block_count; ++i) {
		const size_t display_idx = i + 1;
		_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "%zu", display_idx);
		const auto len = GetPrivateProfileStringA(
			"URL_block",
			shared_buffer,
			"",
			cef_block_list[i],
			MAX_URL_LEN,
			CONFIG_FILEA
		);
		if (0 == len) {
			break;
		}
		_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "Load block url %zu:%s", display_idx, cef_block_list[i]);
		log_debug(shared_buffer);
	}
	log_info("do_hook_cef_url: patch applied.");
}

void hook_cef_url() noexcept
{
	if (true == is_cef_url_hook()) {
		HMODULE libcef_dll_handle = 
			LoadLibraryW(L"libcef.dll");
		if (!libcef_dll_handle) {
			log_debug("Failed to load libcef.dll for for hooking.");
			return;
		}
		do_hook_cef_url(libcef_dll_handle);
	}
}
