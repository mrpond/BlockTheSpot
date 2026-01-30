#include "cef_zip_reader_hook.h"
#include "loader.h"
#include "funct_pointer.h"
#include "log_thread.h"
#include "pattern.h"

static inline size_t cef_buffer_modify_count = 0;
static inline char cef_buffer_list[MAX_CEF_BUFFER_MODIFY_LIST][MAX_URL_LEN] = {};
static inline Modify cef_buffer_modify_patterns[MAX_CEF_BUFFER_MODIFY_LIST][MAX_CEF_BUFFER_MODIFY_LIST][2] = {};

using cef_zip_reader_create_t = void* (*)(void* stream);
static inline cef_zip_reader_create_t cef_zip_reader_create_orig = nullptr;

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

static inline bool EAT_hook_cef_zip_reader(HMODULE libcef_dll_handle) noexcept
{
	HMODULE module = libcef_dll_handle;
	if (!module) return false;

	if (nullptr == ImageDirectoryEntryToDataEx) {
		log_debug("EAT_hook_cef_zip_reader: ImageDirectoryEntryToDataEx is null.");
		return false;
	}

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

		if (0 == lstrcmpiA(name, "cef_zip_reader_create")) {
			DWORD* func_rva = &function[ordinal[i]];
			DWORD rva = *func_rva;

			cef_zip_reader_create_orig =
				reinterpret_cast<cef_zip_reader_create_t>(
					reinterpret_cast<BYTE*>(module) + rva);

			DWORD oldProtect;
			VirtualProtect(func_rva, sizeof(DWORD), PAGE_READWRITE, &oldProtect);
			*func_rva = static_cast<DWORD>(
				reinterpret_cast<BYTE*>(cef_zip_reader_create_hook) -
				reinterpret_cast<BYTE*>(module));
			VirtualProtect(func_rva, sizeof(DWORD), oldProtect, &oldProtect);
			return true;
		}
	}
	return false;
}

static inline void do_hook_cef_zip_reader(HMODULE libcef_dll_handle) noexcept
{
	if (false == EAT_hook_cef_zip_reader(libcef_dll_handle)) {
		log_debug("do_hook_cef_zip_reader: EAT_hook_cef_zip_reader failed.");
		return;
	}
	log_debug("do_hook_cef_zip_reader: EAT_hook_cef_zip_reader done.");
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
		do_hook_cef_zip_reader(libcef_dll_handle);
	}
}
