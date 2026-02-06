#include "cef_url_hook.h"
#include "loader.h"
#include "funct_pointer.h"
#include "log_thread.h"
#include "IAT_hook.h"

static inline size_t cef_block_count = 0;
static inline char cef_block_list[MAX_CEF_BLOCK_LIST][MAX_URL_LEN] = {};

using cef_urlrequest_create_t = void* (*)(void* request, void* client, void* request_context);
static inline cef_urlrequest_create_t cef_urlrequest_create_orig = nullptr;
static inline cef_urlrequest_create_t cef_urlrequest_create_impl = nullptr;

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

void* cef_urlrequest_create_stub(void* request, void* client, void* request_context)
{
	return cef_urlrequest_create_impl(request, client, request_context);
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
	cef_request_get_url_t get_url = get_funct_t<cef_request_get_url_t>(
		request, CEF_REQUEST_GET_URL_OFFSET);

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
	auto is_enable = GetPrivateProfileIntA(
		"URL_block",
		"Enable",
		0,
		CONFIG_FILEA
	);
	return 0 != is_enable;
}

static inline void do_hook_cef_url(HMODULE libcef_dll_handle) noexcept
{
	cef_urlrequest_create_impl = cef_urlrequest_create_hook;
	log_debug("do_hook_cef_url: cef_urlrequest_create_impl = cef_urlrequest_create_hook.");

	cef_string_userfree_utf16_free_orig = reinterpret_cast<cef_string_userfree_utf16_free_t>(
		GetProcAddress_orig(
			libcef_dll_handle,
			"cef_string_userfree_utf16_free"
		));
	log_info("do_hook_cef_url: patch applied.");
}

static inline void load_cef_url_config()
{
	CEF_REQUEST_GET_URL_OFFSET
		= GetPrivateProfileIntA(
			"LIBCEF",
			"CEF_REQUEST_GET_URL_OFFSET",
			static_cast<INT>(CEF_REQUEST_GET_URL_OFFSET),
			CONFIG_FILEA
		);

	for (size_t i = 0; i < MAX_CEF_BLOCK_LIST; ++i) {
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
			_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "Load block url %zu: fail, stop processing", display_idx);
			log_debug(shared_buffer);
			cef_block_count = i;
			break;
		}
		_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "Load block url %zu:%s", display_idx, cef_block_list[i]);
		log_debug(shared_buffer);
	}
	_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "%zu block url loaded", cef_block_count);
	log_info(shared_buffer);
}

void hook_cef_url(HMODULE libcef_dll_handle) noexcept
{
	cef_urlrequest_create_orig =
		reinterpret_cast<cef_urlrequest_create_t>(
			GetProcAddress_orig(libcef_dll_handle, "cef_urlrequest_create"));
	cef_urlrequest_create_impl = cef_urlrequest_create_orig;

	if (true == is_cef_url_hook()) {
		load_cef_url_config();
		do_hook_cef_url(libcef_dll_handle);
	}
}
