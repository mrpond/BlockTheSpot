#include "loader.h"
#include "kill_crashpad.h"

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

		HANDLE thread = OpenThread(
			THREAD_SET_CONTEXT,
			FALSE,
			GetCurrentThreadId()
		);
		
		if (thread) {
			QueueUserAPC(
				bts_main,
				thread,
				reinterpret_cast<ULONG_PTR>(cmd)
			);
			CloseHandle(thread);
		}
		// Crashpad process
		if (NULL != wcsstr(cmd, L"--url=")) {
			kill_crashpad();
		}
	}
	return TRUE;
}
