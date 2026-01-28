#include "cef_url_hook.h"
#include "loader.h"
#include "funct_pointer.h"
#include "log_thread.h"

static inline size_t cef_block_count = 0;
static inline char cef_block_list[MAX_CEF_BLOCK_LIST][MAX_URL_LEN] = {};

using cef_urlrequest_create_t = void* (*)(void* request, void* client, void* request_context);
static inline cef_urlrequest_create_t cef_urlrequest_create_orig = nullptr;

using cef_string_userfree_utf16_free_t = void (*)(void* str);
static inline cef_string_userfree_utf16_free_t cef_string_userfree_utf16_free_orig = nullptr;

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
static inline void* cef_urlrequest_create_hook(struct _cef_request_t* request, void* client, void* request_context)
#else
static inline void* cef_urlrequest_create_hook(void* request, void* client, void* request_context)
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

	const auto len = WideCharToMultiByte(CP_ACP, 0, url, -1, shared_buffer, SHARED_BUFFER_SIZE, NULL, NULL);
	cef_string_userfree_utf16_free_orig((void*)url_utf16);

	if (0 == len) {
		return cef_urlrequest_create_orig(request, client, request_context);
	}

	const bool blocked = is_blocked(shared_buffer);

	char log_buf[256]{};
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
	cef_block_count = GetPrivateProfileIntA(
		"Config",
		"URL_block",
		0,
		CONFIG_FILEA
	);
	if (cef_block_count > MAX_CEF_BLOCK_LIST) {
		_snprintf_s(
			shared_buffer,
			SHARED_BUFFER_SIZE,
			_TRUNCATE, 
			"is_cef_url_hook: cef_block_count %zu exceed limit, set to %zu.",
			cef_block_count,
			MAX_CEF_BLOCK_LIST
		);
		cef_block_count = MAX_CEF_BLOCK_LIST;
		log_info(shared_buffer);
	}
	return 0 != cef_block_count;
}

// https://www.ired.team/offensive-security/code-injection-process-injection/import-adress-table-iat-hooking
static inline bool EAT_hook_cef_url(HMODULE libcef_dll_handle) noexcept
{
	HMODULE module = libcef_dll_handle;
	if (!module) return false;

	HMODULE dbghelp_dll = GetModuleHandleW(L"dbghelp.dll");
	if (!dbghelp_dll) {
		dbghelp_dll = LoadLibraryW(L"dbghelp.dll");
	}
	if (!dbghelp_dll)
		return false;

	ULONG size = 0;
	PIMAGE_EXPORT_DIRECTORY export_dir =
		reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(ImageDirectoryEntryToDataEx(
			module,
			TRUE, // image is loaded in memory
			IMAGE_DIRECTORY_ENTRY_EXPORT,
			&size,
			NULL
		));

	if (nullptr == export_dir) {
		return false;
	}

	auto names = reinterpret_cast<DWORD*>(
		reinterpret_cast<BYTE*>(module) + export_dir->AddressOfNames
		);
	auto ordinal = reinterpret_cast<WORD*>(reinterpret_cast<BYTE*>(module) +
		export_dir->AddressOfNameOrdinals
		);
	auto function = reinterpret_cast<DWORD*>(
		reinterpret_cast<BYTE*>(module) + export_dir->AddressOfFunctions
		);

	for (DWORD i = 0; i < export_dir->NumberOfNames; ++i) {
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
			return true;
		}
	}
	return false;
}

static inline void do_hook_cef_url(HMODULE libcef_dll_handle) noexcept
{
	if (false == EAT_hook_cef_url(libcef_dll_handle)) {
		log_debug("do_hook_cef_url: EAT_hook_cef_url failed.");
		return;
	}
	log_debug("do_hook_cef_url: EAT_hook_cef_url done.");
	cef_string_userfree_utf16_free_orig = reinterpret_cast<cef_string_userfree_utf16_free_t>(
		GetProcAddress(
			libcef_dll_handle,
			"cef_string_userfree_utf16_free"
		));
	log_info("do_hook_cef_url: patch applied.");
}

static inline void load_cef_url_config()
{
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
			_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "Load block url at %zu fail, stop processing", display_idx);
			log_debug(shared_buffer);
			cef_block_count = i;
			break;
		}
		_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "Load block url %zu:%s", display_idx, cef_block_list[i]);
		log_debug(shared_buffer);
	}
}

void hook_cef_url() noexcept
{
	if (true == is_cef_url_hook()) {
		if (nullptr == libcef_dll_handle) {
			libcef_dll_handle =
				LoadLibraryW(L"libcef.dll");
		}
		if (!libcef_dll_handle) {
			log_debug("hook_cef_url: Failed to load libcef.dll.");
			return;
		}
		load_cef_url_config();
		do_hook_cef_url(libcef_dll_handle);
	}
}
