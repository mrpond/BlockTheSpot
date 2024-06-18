#include "pch.h"

#ifdef _WIN64
extern "C"
#endif
LPVOID __stdcall LoadAPI(const char* api_name)
{
    static const std::wstring path{ L"dpapi.dll" };
    static std::unordered_map<std::string, FARPROC> function_map;

    static HMODULE hModule = GetModuleHandleW(path.c_str());
    if (!hModule) {
        hModule = LoadLibraryW(path.c_str());
        if (!hModule) {
            LogError(L"Failed to load library: {}", path);
            return nullptr;
        }
    }

    if (function_map.find(api_name) == function_map.end()) {
        FARPROC proc = GetProcAddress(hModule, api_name);
        if (!proc) {
            LogError(L"Failed to get address of function: {}", Utils::ToString(api_name));
            return nullptr;
        }
        function_map[api_name] = proc;
    }

    return reinterpret_cast<LPVOID>(function_map[api_name]);
}

#ifndef _WIN64
#define API_EXPORT_ORIG(N) \
    static LPVOID _##N = nullptr; \
    static const char S_##N[] = #N; \
    extern "C" __declspec(dllexport) __declspec(naked) void N() \
    { \
        __asm pushad \
        __asm push offset S_##N \
        __asm call LoadAPI \
        __asm mov [_##N], eax \
        __asm popad \
        __asm jmp [_##N] \
    }

API_EXPORT_ORIG(CryptProtectData)
API_EXPORT_ORIG(CryptProtectMemory)
API_EXPORT_ORIG(CryptUnprotectData)
API_EXPORT_ORIG(CryptUnprotectMemory)
API_EXPORT_ORIG(CryptUpdateProtectedState)

//API_EXPORT_ORIG(ClearReportsBetween_ExportThunk)
//API_EXPORT_ORIG(CrashForException_ExportThunk)
//API_EXPORT_ORIG(DisableHook)
//API_EXPORT_ORIG(DrainLog)
//API_EXPORT_ORIG(DumpHungProcessWithPtype_ExportThunk)
//API_EXPORT_ORIG(DumpProcessWithoutCrash)
//API_EXPORT_ORIG(GetApplyHookResult)
//API_EXPORT_ORIG(GetBlockedModulesCount)
//API_EXPORT_ORIG(GetCrashReports_ExportThunk)
//API_EXPORT_ORIG(GetCrashpadDatabasePath_ExportThunk)
//API_EXPORT_ORIG(GetHandleVerifier)
//API_EXPORT_ORIG(GetInstallDetailsPayload)
//API_EXPORT_ORIG(GetUniqueBlockedModulesCount)
//API_EXPORT_ORIG(GetUserDataDirectoryThunk)
//API_EXPORT_ORIG(InjectDumpForHungInput_ExportThunk)
//API_EXPORT_ORIG(IsBrowserProcess)
//API_EXPORT_ORIG(IsCrashReportingEnabledImpl)
//API_EXPORT_ORIG(IsExtensionPointDisableSet)
//API_EXPORT_ORIG(IsThirdPartyInitialized)
//API_EXPORT_ORIG(RegisterLogNotification)
//API_EXPORT_ORIG(RequestSingleCrashUpload_ExportThunk)
//API_EXPORT_ORIG(SetCrashKeyValueImpl)
//API_EXPORT_ORIG(SetMetricsClientId)
//API_EXPORT_ORIG(SetUploadConsent_ExportThunk)
//API_EXPORT_ORIG(SignalChromeElf)
//API_EXPORT_ORIG(SignalInitializeCrashReporting)

#endif

//void ClearReportsBetween_ExportThunk(time_t begin, time_t end) {}
//int CrashForException_ExportThunk(EXCEPTION_POINTERS* info) { return UnhandledExceptionFilter(info); }
//void DisableHook() { }
//uint32_t DrainLog(uint8_t* buffer, uint32_t buffer_size, uint32_t* log_remaining) { return 0; };
//bool DumpHungProcessWithPtype_ExportThunk(HANDLE process_handle, const char* ptype) { return false; }
//void DumpProcessWithoutCrash(void* task_port) {}
//int32_t GetApplyHookResult() { return 0; }
//uint32_t GetBlockedModulesCount() { return 0; }
//size_t GetCrashReports_ExportThunk(void* reports, size_t reports_size) { return 0; }
//const wchar_t* GetCrashpadDatabasePath_ExportThunk() { return nullptr; }
//void* GetHandleVerifier() { return NULL; };
//const void* GetInstallDetailsPayload() { return nullptr; }
//uint32_t GetUniqueBlockedModulesCount() { return 0; }
//bool GetUserDataDirectoryThunk(wchar_t* user_data_dir, size_t user_data_dir_length, wchar_t* invalid_user_data_dir, size_t invalid_user_data_dir_length) { return true; }
//HANDLE InjectDumpForHungInput_ExportThunk(HANDLE process) { return nullptr; }
//bool IsBrowserProcess() { return true; }
//bool IsCrashReportingEnabledImpl(const void* local_state) { return false; }
//bool IsExtensionPointDisableSet() { return false; }
//bool IsThirdPartyInitialized() { return false; }
//bool RegisterLogNotification(HANDLE event_handle) { return false; }
//void RequestSingleCrashUpload_ExportThunk(const char* local_id) {}
//void SetCrashKeyValueImpl(const char*, const char*) {}
//void SetMetricsClientId(const char* client_id) {}
//void SetUploadConsent_ExportThunk(bool consent) {}
//void SignalChromeElf() {}
//void SignalInitializeCrashReporting() {}

#define API_COPY(M, N) \
	_##N = GetProcAddress(M, #N);