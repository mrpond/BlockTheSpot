#include "loader.h"
#include "kill_crashpad.h"
#include "log_thread.h"

// QueueUserAPC via DLL Main instead of CreateThread
// Ready for loader in the future.
BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	if (DLL_PROCESS_ATTACH == ul_reason_for_call) {
		DisableThreadLibraryCalls(hModule);
		LPWSTR cmd = GetCommandLineW();
#ifdef USE_THREAD
		HANDLE thread = CreateThread(
			nullptr,
			0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(bts_main),
			reinterpret_cast<LPVOID>(cmd),
			0,
			nullptr
		);
		if (thread) {
			CloseHandle(thread);
		}
#elif defined(USE_APC)
		QueueUserAPC(
			bts_main,
			GetCurrentThread(),
			reinterpret_cast<ULONG_PTR>(cmd)
		);
#else
		bts_main(reinterpret_cast<ULONG_PTR>(cmd));
#endif
		// Crashpad process
		if (cmd != NULL) {
			if (NULL != wcsstr(cmd, L"--url=")) {
				kill_crashpad();
			}
		}
	}
	if (DLL_PROCESS_DETACH == ul_reason_for_call) {
		LPWSTR cmd = GetCommandLineW();
		if (NULL == wcsstr(cmd, L"--type=") &&
			NULL == wcsstr(cmd, L"--url=")) {
			stop_log();
			//remove_debug_log();
		}
	}
	return TRUE;
}
