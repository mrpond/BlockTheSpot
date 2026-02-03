#include "loader.h"
#include "developer_mode.h"
#include "IAT_hook.h"
#include "kill_crashpad.h"
#include "log_thread.h"
#include "cef_url_hook.h"
#include "cef_zip_reader_hook.h"

static inline bool remove_unused_dll() noexcept
{
	wchar_t old_dpapi[MAX_PATH];
	DWORD len = GetCurrentDirectoryW(MAX_PATH, old_dpapi);
	if (len > 0 && len < MAX_PATH) {
		wcscat_s(old_dpapi, L"\\dpapi.dll");
		return DeleteFileW(old_dpapi);
	}
	return false;
}

const wchar_t* filename_from_path(const wchar_t* path) noexcept
{
	if (!path) return nullptr;

	const wchar_t* last = path;
	for (const wchar_t* p = path; *p; ++p) {
		if (*p == L'\\' || *p == L'/')
			last = p + 1;
	}
	return last;
}

static inline void get_ImageDirectoryEntryToDataEx() noexcept
{
	auto dbghelp_dll_handle = GetModuleHandleW(L"dbghelp.dll");
	if (!dbghelp_dll_handle) {
		dbghelp_dll_handle = LoadLibraryW(L"dbghelp.dll");
	}
	if (!dbghelp_dll_handle) {
		OutputDebugStringW(L"Failed to load dbghelp.dll\n");
		return;
	}

	ImageDirectoryEntryToDataEx =
		reinterpret_cast<ImageDirectoryEntryToDataEx_t>(
			GetProcAddress(dbghelp_dll_handle, "ImageDirectoryEntryToDataEx")
			);

	if (nullptr == ImageDirectoryEntryToDataEx) {
		OutputDebugStringW(L"Failed to get ImageDirectoryEntryToDataEx address\n");
		return;
	}
}

VOID CALLBACK bts_main(ULONG_PTR param)
{
	get_ImageDirectoryEntryToDataEx();
	IAT_hook_GetProcAddress();
	const wchar_t* cmd =
		reinterpret_cast<const wchar_t*>(param);
	//  Spotify's main process
	if (NULL == wcsstr(cmd, L"--type=") &&
		NULL == wcsstr(cmd, L"--url=")) {
		init_log_thread();
		if (true == remove_unused_dll()) {
			log_debug("Remove unused dpapi.dll.");
		}
		//LoadLibraryW(L".\\Users\\dpapi.dll");
		//log_debug("dpapi loaded.");
		HMODULE spotify_dll_handle =
			LoadLibraryW(L"spotify.dll");
		HMODULE libcef_dll_handle =
			LoadLibraryW(L"libcef.dll");

		if (!spotify_dll_handle) {
			log_debug("Failed to load spotify.dll for developer mode hooking.");
			return;
		}
		if (!libcef_dll_handle) {
			log_debug("Failed to load libcef.dll.");
			return;
		}
		hook_developer_mode(spotify_dll_handle);
		IAT_hook_GetProcAddress(spotify_dll_handle);
		hook_cef_url(libcef_dll_handle);
		hook_cef_reader(libcef_dll_handle);	// not finished yet.
		// FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
		log_info("Loader initialized successfully.");
	}
}
