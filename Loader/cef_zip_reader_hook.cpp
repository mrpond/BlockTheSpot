#include "cef_zip_reader_hook.h"
#include "loader.h"
#include "funct_pointer.h"
#include "log_thread.h"
#include "pattern.h"
#include "IAT_hook.h"

static inline size_t cef_buffer_modify_count = 0;
static inline char cef_buffer_list[MAX_CEF_BUFFER_MODIFY_LIST][MAX_URL_LEN] = {};
static inline Modify cef_buffer_modify_patterns[MAX_CEF_BUFFER_MODIFY_LIST][MAX_CEF_BUFFER_MODIFY_LIST][2] = {};

using cef_zip_reader_create_t = void* (*)(void* stream);
static inline cef_zip_reader_create_t cef_zip_reader_create_orig = nullptr;
static inline cef_zip_reader_create_t cef_zip_reader_create_impl = nullptr;

using cef_zip_reader_read_file_t = int(CALLBACK*)(void* self, void* buffer, size_t bufferSize);
static cef_zip_reader_read_file_t cef_zip_reader_read_file_orig = nullptr;

static bool need_patch(const char* in_file) noexcept {
	for (size_t i = 0; i < cef_buffer_modify_count; ++i) {
		const char* target = cef_buffer_list[i];

		if (0 == lstrcmpiA(in_file, target)) {
			return true;
		}
	}
	return false;
}

#ifdef USE_LIBCEF
int CALLBACK cef_zip_reader_t_read_file_hook(struct _cef_zip_reader_t* self, void* buffer, size_t bufferSize)
#else
int CALLBACK cef_zip_reader_read_file_hook(void* self, void* buffer, size_t bufferSize)
#endif
{
	int _retval = cef_zip_reader_read_file_orig(self, buffer, bufferSize);

#ifdef USE_LIBCEF
	std::wstring file_name = Utils::ToString(self->get_file_name(self)->str);
#else
	using get_file_name_t = void* (__stdcall*)(void*);
	const auto get_file_name = get_funct_t<get_file_name_t>(
		self, CEF_ZIP_READER_GET_FILE_NAME_OFFSET);
	const wchar_t* file_name = *reinterpret_cast<wchar_t**>(get_file_name(self));
#endif
	const auto len = WideCharToMultiByte(CP_ACP, 0, file_name, -1, shared_buffer, SHARED_BUFFER_SIZE, NULL, NULL);
	if (0 == len) {
		return _retval;
	}
	const bool patch = need_patch(shared_buffer);

	char log_buf[256]{};
	_snprintf_s(
		log_buf,
		sizeof(log_buf),
		_TRUNCATE,
		"zip_reader: %s %s",
		patch ? "patch" : "read",
		shared_buffer
	);
	log_debug(log_buf);

	return _retval;
}

void* cef_zip_reader_create_stub(void* stream)
{
	return cef_zip_reader_create_impl(stream);
}

#ifdef USE_LIBCEF
cef_zip_reader_t* cef_zip_reader_create_hook(cef_stream_reader_t* stream)
#else
void* cef_zip_reader_create_hook(void* stream)
#endif
{
#ifdef USE_LIBCEF
	cef_zip_reader_t* zip_reader = (cef_zip_reader_t*)cef_zip_reader_create_orig(stream);
	cef_zip_reader_t_read_file_orig = (_cef_zip_reader_t_read_file)zip_reader->read_file;
#else
	auto zip_reader = cef_zip_reader_create_orig(stream);
	cef_zip_reader_read_file_orig = 
		get_funct_t<cef_zip_reader_read_file_t>(
			zip_reader, CEF_ZIP_READER_GET_READ_FILE_OFFSET);
	overwrite_funct_t<cef_zip_reader_read_file_t>(
		zip_reader, CEF_ZIP_READER_GET_READ_FILE_OFFSET, cef_zip_reader_read_file_hook);
#endif
	return zip_reader;
}

static inline void do_hook_cef_zip_reader(HMODULE libcef_dll_handle) noexcept
{
	cef_zip_reader_create_impl = cef_zip_reader_create_hook;
	log_debug("do_hook_cef_zip_reader: cef_zip_reader_create_impl = cef_zip_reader_create_hook.");
	log_info("do_hook_cef_zip_reader: patch applied.");
}

static inline void load_cef_reader_config()
{
	CEF_ZIP_READER_GET_READ_FILE_OFFSET = GetPrivateProfileIntA(
		"LIBCEF",
		"CEF_ZIP_READER_GET_READ_FILE_OFFSET",
		static_cast<INT>(CEF_ZIP_READER_GET_READ_FILE_OFFSET),
		CONFIG_FILEA
	);

	CEF_ZIP_READER_GET_FILE_NAME_OFFSET = GetPrivateProfileIntA(
		"LIBCEF",
		"CEF_ZIP_READER_GET_FILE_NAME_OFFSET",
		static_cast<INT>(CEF_ZIP_READER_GET_FILE_NAME_OFFSET),
		CONFIG_FILEA
	);

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
		"Buffer_modify",
		"Count",
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

void hook_cef_reader(HMODULE libcef_dll_handle) noexcept
{
	cef_zip_reader_create_orig = reinterpret_cast<cef_zip_reader_create_t>(
		GetProcAddress_orig(libcef_dll_handle, "cef_zip_reader_create"));
	cef_zip_reader_create_impl = cef_zip_reader_create_orig;

	if (true == is_cef_reader_hook()) {
		load_cef_reader_config();
		do_hook_cef_zip_reader(libcef_dll_handle);
	}
}
