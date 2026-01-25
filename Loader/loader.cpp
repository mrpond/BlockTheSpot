#include "loader.h"
#include "developer_mode.h"
#include "IAT_hook.h"
#include "kill_crashpad.h"
#include "log_thread.h"

bool remove_unused_dll() noexcept
{
	wchar_t old_dpapi[MAX_PATH];
	DWORD len = GetCurrentDirectoryW(MAX_PATH, old_dpapi);
	if (len > 0 && len < MAX_PATH) {
		wcscat_s(old_dpapi, L"\\dpapi.dll");
		log_debug("Removing unused dpapi.dll");
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

void CALLBACK bts_main(ULONG_PTR param)
{
	IAT_hook_GetProcAddress();
	const wchar_t* cmd =
		reinterpret_cast<const wchar_t*>(param);
	//  Spotify's main process
	if (NULL == wcsstr(cmd, L"--type=") &&
		NULL == wcsstr(cmd, L"--url=")) {
		init_log_thread();

		hook_developer_mode();
		remove_unused_dll();
		//hook_developer_mode();
		LoadLibraryW(L".\\Users\\dpapi.dll");
		log_debug("dpapi loaded.");
		log_info("Loader initialized successfully.");
	}
}
