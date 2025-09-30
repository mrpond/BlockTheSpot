#define WIN32_LEAN_AND_MEAN
#define NOCRYPT
#include <windows.h>
#include <unordered_map>
#include <string>

extern "C" FARPROC __stdcall GetFunction(const char* n) noexcept {
    static constexpr auto l = L"dpapi.dll";
    static HMODULE m = [] { HMODULE h = GetModuleHandleW(l); return h ? h : LoadLibraryW(l); }();
    if (!m) return nullptr;
    static std::unordered_map<std::string, FARPROC> c;
    auto& p = c[n];
    return p ? p : (p = GetProcAddress(m, n));
}

#ifndef _WIN64
#if defined(__GNUC__) || defined(__clang__)
#define PROXY_API(N) \
        static void* _##N = nullptr; \
        static const char S_##N[] = #N; \
        extern "C" __attribute__((naked)) void N() \
        { \
            __asm__ volatile ( \
                "pushl %%ebp\n\t" \
                "movl %%esp, %%ebp\n\t" \
                "pushl %%eax\n\t" \
                "pushl %0\n\t" \
                "call _GetFunction@4\n\t" \
                "addl $4, %%esp\n\t" \
                "movl %%eax, %1\n\t" \
                "popl %%eax\n\t" \
                "movl %%ebp, %%esp\n\t" \
                "popl %%ebp\n\t" \
                "jmp *%1\n\t" \
                : \
                : "r" (S_##N), "m" (_##N) \
                : "%eax", "memory" \
            ); \
        }
#else
#define PROXY_API(N) \
        static LPVOID _##N = nullptr; \
        static const char S_##N[] = #N; \
        extern "C" __declspec(dllexport) __declspec(naked) void N() \
        { \
            __asm { \
                pushad \
                push offset S_##N \
                call GetFunction \
                mov [_##N], eax \
                popad \
                jmp [_##N] \
            } \
        }
#endif

PROXY_API(CryptProtectData)
PROXY_API(CryptProtectMemory)
PROXY_API(CryptUnprotectData)
PROXY_API(CryptUnprotectMemory)
PROXY_API(CryptUpdateProtectedState)

// PROXY_API(ClearReportsBetween_ExportThunk)
// PROXY_API(CrashForException_ExportThunk)
// PROXY_API(DisableHook)
// PROXY_API(DrainLog)
// PROXY_API(DumpHungProcessWithPtype_ExportThunk)
// PROXY_API(DumpProcessWithoutCrash)
// PROXY_API(GetApplyHookResult)
// PROXY_API(GetBlockedModulesCount)
// PROXY_API(GetCrashReports_ExportThunk)
// PROXY_API(GetCrashpadDatabasePath_ExportThunk)
// PROXY_API(GetHandleVerifier)
// PROXY_API(GetInstallDetailsPayload)
// PROXY_API(GetUniqueBlockedModulesCount)
// PROXY_API(GetUserDataDirectoryThunk)
// PROXY_API(InjectDumpForHungInput_ExportThunk)
// PROXY_API(IsBrowserProcess)
// PROXY_API(IsCrashReportingEnabledImpl)
// PROXY_API(IsExtensionPointDisableSet)
// PROXY_API(IsTemporaryUserDataDirectoryCreatedForHeadless)
// PROXY_API(IsThirdPartyInitialized)
// PROXY_API(RegisterLogNotification)
// PROXY_API(RequestSingleCrashUpload_ExportThunk)
// PROXY_API(SetCrashKeyValueImpl)
// PROXY_API(SetMetricsClientId)
// PROXY_API(SetUploadConsent_ExportThunk)
// PROXY_API(SignalChromeElf)
// PROXY_API(SignalInitializeCrashReporting)
#endif

// void ClearReportsBetween_ExportThunk(time_t begin, time_t end) {}
// int CrashForException_ExportThunk(EXCEPTION_POINTERS* info) { return UnhandledExceptionFilter(info); }
// void DisableHook() { }
// uint32_t DrainLog(uint8_t* buffer, uint32_t buffer_size, uint32_t* log_remaining) { return 0; };
// bool DumpHungProcessWithPtype_ExportThunk(HANDLE process_handle, const char* ptype) { return false; }
// void DumpProcessWithoutCrash(void* task_port) {}
// int32_t GetApplyHookResult() { return 0; }
// uint32_t GetBlockedModulesCount() { return 0; }
// size_t GetCrashReports_ExportThunk(void* reports, size_t reports_size) { return 0; }
// const wchar_t* GetCrashpadDatabasePath_ExportThunk() { return nullptr; }
// void* GetHandleVerifier() { return nullptr; }
// const void* GetInstallDetailsPayload() { return nullptr; }
// uint32_t GetUniqueBlockedModulesCount() { return 0; }
// bool GetUserDataDirectoryThunk(wchar_t* user_data_dir, size_t user_data_dir_length, wchar_t* invalid_user_data_dir, size_t invalid_user_data_dir_length) { return true; }
// HANDLE InjectDumpForHungInput_ExportThunk(HANDLE process) { return nullptr; }
// bool IsBrowserProcess() { return true; }
// bool IsCrashReportingEnabledImpl(const void* local_state) { return false; }
// bool IsExtensionPointDisableSet() { return false; }
// bool IsTemporaryUserDataDirectoryCreatedForHeadless() { return false; }
// bool IsThirdPartyInitialized() { return false; }
// bool RegisterLogNotification(HANDLE event_handle) { return false; }
// void RequestSingleCrashUpload_ExportThunk(const char* local_id) {}
// void SetCrashKeyValueImpl(const char*, const char*) {}
// void SetMetricsClientId(const char* client_id) {}
// void SetUploadConsent_ExportThunk(bool consent) {}
// void SignalChromeElf() {}
// void SignalInitializeCrashReporting() {}