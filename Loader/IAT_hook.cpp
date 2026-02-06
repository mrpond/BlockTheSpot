#include "IAT_hook.h"
#include "WinTrust_hook.h"

static FARPROC WINAPI GetProcAddress_hook(HMODULE hModule, LPCSTR lpProcName)
{
	if (!lpProcName || 0 == HIWORD(lpProcName))
		return GetProcAddress_orig(hModule, lpProcName);

	if (0 == lstrcmpiA(lpProcName, "WinVerifyTrust")) {
		if (hModule == GetModuleHandleW(L"WinTrust.dll")) {
			return reinterpret_cast<FARPROC>(WinVerifyTrust_hook);
		}
	}

	return GetProcAddress_orig(hModule, lpProcName);
}

// https://www.ired.team/offensive-security/code-injection-process-injection/import-adress-table-iat-hooking
bool process_IAT_hook_GetProcAddress(HMODULE module) noexcept
{
	if (!module) return false;

	if (nullptr == ImageDirectoryEntryToDataEx) {
		OutputDebugStringW(L"process_IAT_hook_GetProcAddress: ImageDirectoryEntryToDataEx is null.");
		return false;
	}

	ULONG size = 0;
	PIMAGE_IMPORT_DESCRIPTOR imports =
		reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(ImageDirectoryEntryToDataEx(
			module,
			TRUE, // image is loaded in memory
			IMAGE_DIRECTORY_ENTRY_IMPORT,
			&size,
			NULL
		));

	if (nullptr == imports) {
		return false;
	}

	for (; imports->Name; ++imports) {
		LPCSTR dll_name = reinterpret_cast<LPCSTR>(
			reinterpret_cast<BYTE*>(module) + imports->Name
			);

		// GetProcAddress is in kernel32
		if (0 == lstrcmpiA(dll_name, "kernel32.dll")) {

			PIMAGE_THUNK_DATA thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
				reinterpret_cast<BYTE*>(module) + imports->FirstThunk
				);

			for (; thunk->u1.Function; ++thunk) {
				PROC* func = reinterpret_cast<PROC*>(&thunk->u1.Function);

				if (*func == reinterpret_cast<PROC>(GetProcAddress)) {
					DWORD oldProtect;
					VirtualProtect(func, sizeof(PROC), PAGE_READWRITE, &oldProtect);

					GetProcAddress_orig = reinterpret_cast<GetProcAddress_t>(*func);
					*func = reinterpret_cast<PROC>(GetProcAddress_hook);

					VirtualProtect(func, sizeof(PROC), oldProtect, &oldProtect);
					return true;
				}
			}
		}
	}
	return false;
}
//
//bool IAT_unhook_GetProcAddress() noexcept
//{
//	HMODULE module = GetModuleHandleW(nullptr);
//	if (!module) return false;
//
//	PIMAGE_DOS_HEADER dos = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
//	PIMAGE_NT_HEADERS nt = reinterpret_cast<PIMAGE_NT_HEADERS>(
//		reinterpret_cast<BYTE*>(module) + dos->e_lfanew
//		);
//
//	IMAGE_DATA_DIRECTORY& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
//	if (!dir.VirtualAddress) return false;
//
//	PIMAGE_IMPORT_DESCRIPTOR imports = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
//		reinterpret_cast<BYTE*>(module) + dir.VirtualAddress
//		);
//
//	for (; imports->Name; ++imports) {
//		LPCSTR dll_name = reinterpret_cast<LPCSTR>(
//			reinterpret_cast<BYTE*>(module) + imports->Name
//			);
//
//		// GetProcAddress is in kernel32
//		if (0 == lstrcmpiA(dll_name, "kernel32.dll")) {
//
//			PIMAGE_THUNK_DATA thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
//				reinterpret_cast<BYTE*>(module) + imports->FirstThunk
//				);
//
//			for (; thunk->u1.Function; ++thunk) {
//				PROC* func = reinterpret_cast<PROC*>(&thunk->u1.Function);
//
//				if (*func == reinterpret_cast<PROC>(GetProcAddress_hook)) {
//					DWORD oldProtect;
//					VirtualProtect(func, sizeof(PROC), PAGE_READWRITE, &oldProtect);
//					*func = reinterpret_cast<PROC>(GetProcAddress_orig);
//					VirtualProtect(func, sizeof(PROC), oldProtect, &oldProtect);
//				}
//			}
//		}
//	}
//	return true;
//}
