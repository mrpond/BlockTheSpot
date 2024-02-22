IFDEF RAX ; # 64-bit

    PUSH_ALL MACRO
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

    POP_ALL MACRO
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

    EXTERNDEF LoadAPI:PROC

    API_EXPORT_ORIG MACRO API_NAME:REQ
        .DATA
            _&API_NAME QWORD 0
            S_&API_NAME DB '&API_NAME', 0
        .CODE
        &API_NAME PROC
            PUSH_ALL
            sub rsp, 8
            mov rcx, OFFSET S_&API_NAME
            call LoadAPI
            mov _&API_NAME, rax
            add rsp, 8
            POP_ALL
            jmp QWORD PTR [_&API_NAME]
        &API_NAME ENDP
    ENDM

    API_EXPORT_ORIG CryptProtectData
    API_EXPORT_ORIG CryptProtectMemory
    API_EXPORT_ORIG CryptUnprotectData
    API_EXPORT_ORIG CryptUnprotectMemory
    API_EXPORT_ORIG CryptUpdateProtectedState
    
    ; API_EXPORT_ORIG ClearReportsBetween_ExportThunk
    ; API_EXPORT_ORIG CrashForException_ExportThunk
    ; API_EXPORT_ORIG DisableHook
    ; API_EXPORT_ORIG DrainLog
    ; API_EXPORT_ORIG DumpHungProcessWithPtype_ExportThunk
    ; API_EXPORT_ORIG DumpProcessWithoutCrash
    ; API_EXPORT_ORIG GetApplyHookResult
    ; API_EXPORT_ORIG GetBlockedModulesCount
    ; API_EXPORT_ORIG GetCrashReports_ExportThunk
    ; API_EXPORT_ORIG GetCrashpadDatabasePath_ExportThunk
    ; API_EXPORT_ORIG GetHandleVerifier
    ; API_EXPORT_ORIG GetInstallDetailsPayload
    ; API_EXPORT_ORIG GetUniqueBlockedModulesCount
    ; API_EXPORT_ORIG GetUserDataDirectoryThunk
    ; API_EXPORT_ORIG InjectDumpForHungInput_ExportThunk
    ; API_EXPORT_ORIG IsBrowserProcess
    ; API_EXPORT_ORIG IsCrashReportingEnabledImpl
    ; API_EXPORT_ORIG IsExtensionPointDisableSet
    ; API_EXPORT_ORIG IsThirdPartyInitialized
    ; API_EXPORT_ORIG RegisterLogNotification
    ; API_EXPORT_ORIG RequestSingleCrashUpload_ExportThunk
    ; API_EXPORT_ORIG SetCrashKeyValueImpl
    ; API_EXPORT_ORIG SetMetricsClientId
    ; API_EXPORT_ORIG SetUploadConsent_ExportThunk
    ; API_EXPORT_ORIG SignalChromeElf
    ; API_EXPORT_ORIG SignalInitializeCrashReporting

ENDIF

END
