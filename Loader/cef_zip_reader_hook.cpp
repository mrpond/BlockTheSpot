#include "cef_zip_reader_hook.h"
#include "loader.h"
#include "funct_pointer.h"
#include "log_thread.h"
#include "pattern.h"

static inline size_t cef_buffer_modify_count = 0;
static inline char cef_buffer_list[MAX_CEF_BUFFER_MODIFY_LIST][MAX_URL_LEN] = {};
static inline Modify cef_buffer_modify_patterns[MAX_CEF_BUFFER_MODIFY_LIST][MAX_CEF_BUFFER_MODIFY_LIST * 2] = {};

static inline void load_cef_reader_config()
{
	for (size_t i = 0; i < cef_buffer_modify_count; ++i) {
		const size_t display_idx = i + 1;
		_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "%zu", display_idx);
		const auto len = GetPrivateProfileStringA(
			"Buffer_modify",
			shared_buffer,
			"",
			cef_buffer_list[i],
			MAX_URL_LEN,
			CONFIG_FILEA
		);
		if (0 == len) {
			_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "Load buffer modify at %zu fail, stop processing", display_idx);
			log_debug(shared_buffer);
			cef_buffer_modify_count = i;
			break;
		}
		_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "Load buffer modify %zu:%s", display_idx, cef_buffer_list[i]);
		log_debug(shared_buffer);
	}
}

static inline bool is_cef_reader_hook() noexcept
{
	cef_buffer_modify_count = GetPrivateProfileIntA(
		"Config",
		"Buffer_modify",
		0,
		CONFIG_FILEA
	);
	if (cef_buffer_modify_count > MAX_CEF_BUFFER_MODIFY_LIST) {
		_snprintf_s(
			shared_buffer,
			SHARED_BUFFER_SIZE,
			_TRUNCATE,
			"is_cef_reader_hook: cef_buffer_modify_count %zu exceed limit, set to %zu.",
			cef_buffer_modify_count,
			MAX_CEF_BLOCK_LIST
		);
		cef_buffer_modify_count = MAX_CEF_BUFFER_MODIFY_LIST;
		log_info(shared_buffer);
	}
	return 0 != cef_buffer_modify_count;
}

void hook_cef_reader() noexcept
{
	if (true == is_cef_reader_hook()) {
		if (nullptr == libcef_dll_handle) {
			libcef_dll_handle =
				LoadLibraryW(L"libcef.dll");
		}
		if (!libcef_dll_handle) {
			log_debug("hook_cef_reader: Failed to load libcef.dll.");
			return;
		}
		load_cef_reader_config();
	}
}
