#include "WinTrust_hook.h"

// https://learn.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--verifying-the-signature-of-a-pe-file
LONG WINAPI WinVerifyTrust_hook(HWND hwnd, GUID* pgActionID, LPVOID pWVTData)
{
	static auto file_handle = CreateFileW(ORIGINAL_CHROME_ELF_DLL,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);
	static WINTRUST_FILE_INFO FileInfo = {
		.cbStruct = sizeof(WINTRUST_FILE_INFO_),
		.pcwszFilePath = ORIGINAL_CHROME_ELF_DLL, // hehehehe, hahahahhahaa
		.hFile = file_handle,
	};
	const auto WintrustData = reinterpret_cast<WINTRUST_DATA*>(pWVTData);
	auto pFileInfo = WintrustData->pFile;

	const wchar_t* file_name = filename_from_path(pFileInfo->pcwszFilePath);
	if (0 == lstrcmpiW(L"chrome_elf.dll", file_name)) {
		if (ERROR_SUCCESS != file_handle) {
			WintrustData->pFile = &FileInfo;
			const auto result = WinVerifyTrust(hwnd, pgActionID, pWVTData);
			WintrustData->pFile = pFileInfo;
			return result;
		}
	}
	return WinVerifyTrust(hwnd, pgActionID, pWVTData);
}
