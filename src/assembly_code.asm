IFDEF RAX ; # 64-bit

    SAVE_ALL MACRO
        push rax
        push rbx
        push rcx
        push rdx
        push rbp
        push rsp
        push rsi
        push rdi
        push r8
        push r9
        push r10
        push r11
        push r12
        push r13
        push r14
        push r15
    ENDM

    RESTORE_ALL MACRO
        pop r15
        pop r14
        pop r13
        pop r12
        pop r11
        pop r10
        pop r9
        pop r8
        pop rdi
        pop rsi
        pop rsp
        pop rbp
        pop rdx
        pop rcx
        pop rbx
        pop rax
    ENDM

    EXTERNDEF GetFunction:PROC

    PROXY_API MACRO API_NAME:REQ
        .DATA
            _&API_NAME QWORD 0
            S_&API_NAME DB '&API_NAME', 0
        .CODE
        &API_NAME PROC
            SAVE_ALL
            sub rsp, 8
            mov rcx, OFFSET S_&API_NAME
            call GetFunction
            mov _&API_NAME, rax
            add rsp, 8
            RESTORE_ALL
            jmp QWORD PTR [_&API_NAME]
        &API_NAME ENDP
    ENDM

    PROXY_API CryptProtectData
    PROXY_API CryptProtectMemory
    PROXY_API CryptUnprotectData
    PROXY_API CryptUnprotectMemory
    PROXY_API CryptUpdateProtectedState
    
    ; PROXY_API ClearReportsBetween_ExportThunk
    ; PROXY_API CrashForException_ExportThunk
    ; PROXY_API DisableHook
    ; PROXY_API DrainLog
    ; PROXY_API DumpHungProcessWithPtype_ExportThunk
    ; PROXY_API DumpProcessWithoutCrash
    ; PROXY_API GetApplyHookResult
    ; PROXY_API GetBlockedModulesCount
    ; PROXY_API GetCrashReports_ExportThunk
    ; PROXY_API GetCrashpadDatabasePath_ExportThunk
    ; PROXY_API GetHandleVerifier
    ; PROXY_API GetInstallDetailsPayload
    ; PROXY_API GetUniqueBlockedModulesCount
    ; PROXY_API GetUserDataDirectoryThunk
    ; PROXY_API InjectDumpForHungInput_ExportThunk
    ; PROXY_API IsBrowserProcess
    ; PROXY_API IsCrashReportingEnabledImpl
    ; PROXY_API IsExtensionPointDisableSet
    ; PROXY_API IsTemporaryUserDataDirectoryCreatedForHeadless
    ; PROXY_API IsThirdPartyInitialized
    ; PROXY_API RegisterLogNotification
    ; PROXY_API RequestSingleCrashUpload_ExportThunk
    ; PROXY_API SetCrashKeyValueImpl
    ; PROXY_API SetMetricsClientId
    ; PROXY_API SetUploadConsent_ExportThunk
    ; PROXY_API SignalChromeElf
    ; PROXY_API SignalInitializeCrashReporting

ENDIF

END