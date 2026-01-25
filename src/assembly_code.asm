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

    EXTERNDEF load_api:PROC

    API_EXPORT_ORIG MACRO API_NAME:REQ
        .DATA
            _&API_NAME QWORD 0
            S_&API_NAME DB '&API_NAME', 0
        .CODE
        &API_NAME PROC
            PUSH_ALL
            sub rsp, 8
            mov rcx, OFFSET S_&API_NAME
            call load_api
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

ENDIF

END
