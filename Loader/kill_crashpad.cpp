#include "kill_crashpad.h"
#include "loader.h"

static inline bool is_block_crashpad() noexcept
{
	const auto result = GetPrivateProfileIntW(
		L"LIBCEF",
		L"Block_crashpad",
		0,
		CONFIG_FILEW
	);
	return 0 != result;
}

void kill_crashpad() noexcept
{
	if (is_block_crashpad()) {
		SleepEx(INFINITE, TRUE);
		// This will write something into debug.log
		//ExitProcess(0);
	}

}
