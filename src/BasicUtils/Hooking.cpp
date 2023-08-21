#define ENABLE_DETOURS

#include "Hooking.h"
#include <stdexcept>
#include <mutex>
#ifdef ENABLE_DETOURS
#include <detours.h>
#endif

#include "Utils.h"
#include "Console.h"

std::mutex mtx;

bool Hooking::HookFunction(void** function_pointer, void* hook_function)
{
    std::lock_guard<std::mutex> lock(mtx);

    if (!function_pointer || !hook_function) {
        PrintError(L"HookFunction: Invalid function pointer or hook function.");
        return false;
    }

#ifdef ENABLE_DETOURS
    LONG error = NO_ERROR;

    if ((error = DetourTransactionBegin()) != NO_ERROR) {
        PrintError(L"DetourTransactionBegin error: {}", error);
        return false;
    }

    if ((error = DetourUpdateThread(GetCurrentThread())) != NO_ERROR) {
        PrintError(L"DetourUpdateThread error: {}", error);
        return false;
    }

    if ((error = DetourAttach(function_pointer, hook_function)) != NO_ERROR) {
        PrintError(L"DetourAttach error: {}", error);
        return false;
    }

    if ((error = DetourTransactionCommit()) != NO_ERROR) {
        PrintError(L"DetourTransactionCommit error: {}", error);
        return false;
    }
#else // It is currently in the testing phase. There may be errors.
    auto hook_data_it = std::find_if(hook_data_list.begin(), hook_data_list.end(),
        [function_pointer](const HookData& hook_data) {
            return hook_data.function_pointer == function_pointer;
        });

    if (hook_data_it != hook_data_list.end()) {
        PrintError(L"HookFunction: Function pointer is already hooked.");
        return false;
    }

    HookData hook_data{ .function_pointer = function_pointer, .code = reinterpret_cast<std::uint8_t*>(*function_pointer) };

    if (!BackupFunctionCode(hook_data)) {
        PrintError(L"HookFunction: BackupFunctionCode failed.");
        return false;
    }

    if (!ApplyHook(hook_data, hook_function)) {
        PrintError(L"HookFunction: ApplyHook failed.");
        return false;
    }

    hook_data_list.push_back(hook_data);

    *function_pointer = hook_data.new_code;
#endif
    return true;
}

bool Hooking::UnhookFunction(void** function_pointer, void* hook_function)
{
    std::lock_guard<std::mutex> lock(mtx);
#ifdef ENABLE_DETOURS
    if (!function_pointer || !hook_function) {
        PrintError(L"UnhookFunction: Invalid function pointer or hook function.");
        return false;
    }

    LONG error = NO_ERROR;

    if ((error = DetourTransactionBegin()) != NO_ERROR) {
        PrintError(L"DetourTransactionBegin error: {}", error);
        return false;
    }

    if ((error = DetourUpdateThread(GetCurrentThread())) != NO_ERROR) {
        PrintError(L"DetourUpdateThread error: {}", error);
        return false;
    }

    if ((error = DetourDetach(function_pointer, hook_function)) != NO_ERROR) {
        PrintError(L"DetourDetach error: {}", error);
        return false;
    }

    if ((error = DetourTransactionCommit()) != NO_ERROR) {
        PrintError(L"DetourTransactionCommit error: {}", error);
        return false;
    }
#else
    if (!function_pointer) {
        PrintError(L"UnhookFunction: Invalid function pointer.");
        return false;
    }

    auto hook_data_it = std::find_if(hook_data_list.begin(), hook_data_list.end(),
        [function_pointer](const HookData& hook_data) {
            return hook_data.code == *function_pointer;
        });

    if (hook_data_it == hook_data_list.end()) {
        PrintError(L"UnhookFunction: Function pointer is not hooked.");
        return false;
    }

    if (!RestoreFunctionCode(*hook_data_it)) {
        PrintError(L"UnhookFunction: RestoreFunctionCode failed.");
        return false;
    }

    hook_data_list.erase(hook_data_it);
#endif
    return true;
}

#ifndef ENABLE_DETOURS
bool Hooking::BackupFunctionCode(HookData& hook_data)
{
    std::size_t original_code_size = GetOriginalCodeSize(hook_data.code);
    hook_data.original_code.resize(original_code_size);
    std::copy_n(reinterpret_cast<std::uint8_t*>(*hook_data.function_pointer), original_code_size, hook_data.original_code.begin());

    if (hook_data.original_code.size() < 5) {
        PrintError(L"BackupFunctionCode: Original code size is less than 5 bytes.");
        return false;
    }

    hook_data.new_code = reinterpret_cast<std::uint8_t*>(VirtualAlloc(nullptr, hook_data.original_code.size(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!hook_data.new_code) {
        PrintError(L"BackupFunctionCode: Failed to allocate memory for new code.");
        return false;
    }

    memcpy(hook_data.new_code, hook_data.code, hook_data.original_code.size());

#ifdef _WIN64
    std::uint8_t* offset = hook_data.code + hook_data.original_code.size();
    //hook_data.new_code[hook_data.original_code.size()] = 0xFF;
    //hook_data.new_code[hook_data.original_code.size() + 1] = 0x25;
    //memcpy(hook_data.new_code + hook_data.original_code.size() + 6, &offset, sizeof(offset));
    GenerateIndirectJump(hook_data.new_code + hook_data.original_code.size(), reinterpret_cast<std::uint8_t**>(&offset));
    //Print(L"{:X} => {}", (std::uintptr_t)hook_data.new_code, Utils::ToHexWideString(hook_data.new_code, hook_data.original_code.size() + 12));
#else
    std::uint8_t* offset = hook_data.code + hook_data.original_code.size();
    GenerateImmediateJump(hook_data.new_code + hook_data.original_code.size(), offset);
#endif
    return true;
}

bool Hooking::ApplyHook(HookData& hook_data, void* hook_function)
{
    DWORD old_protect;
    if (!VirtualProtect(hook_data.code, hook_data.original_code.size(), PAGE_EXECUTE_READWRITE, &old_protect)) {
        PrintError(L"ApplyHook: Failed to change memory protection for code.");
        return false;
    }

    auto code = GenerateImmediateJump(hook_data.code, static_cast<std::uint8_t*>(hook_function));
    GenerateBreakpoint(code, hook_data.code + hook_data.original_code.size());

    DWORD new_protect;
    if (!VirtualProtect(hook_data.code, hook_data.original_code.size(), old_protect, &new_protect)) {
        PrintError(L"ApplyHook: Failed to restore memory protection for code.");
        return false;
    }

    return true;
}

bool Hooking::RestoreFunctionCode(HookData& hook_data)
{
    DWORD old_protect;
    if (!VirtualProtect(hook_data.code, hook_data.original_code.size(), PAGE_EXECUTE_READWRITE, &old_protect)) {
        PrintError(L"RestoreFunctionCode: Failed to change memory protection for code.");
        return false;
    }

    memcpy(hook_data.code, hook_data.original_code.data(), hook_data.original_code.size());

    DWORD new_protect;
    if (!VirtualProtect(hook_data.code, hook_data.original_code.size(), old_protect, &new_protect)) {
        PrintError(L"RestoreFunctionCode: Failed to restore memory protection for code.");
        return false;
    }

    if (!VirtualFree(hook_data.new_code, 0, MEM_RELEASE)) {
        PrintError(L"RestoreFunctionCode: Failed to free memory allocated for new code.");
        return false;
    }

    return true;
}

std::size_t Hooking::GetOriginalCodeSize(std::uint8_t* code)
{
    std::size_t original_code_size = 0;
    while (original_code_size < 5) {
        std::uint8_t opcode = code[original_code_size];

        if (opcode >= 0x50 && opcode <= 0x57) {
            original_code_size += 1; // push rXX
        }
        else if (opcode == 0x41 && code[original_code_size + 1] >= 0x50 && code[original_code_size + 1] <= 0x57) {
            original_code_size += 2; // push rXX
        }
        else if (opcode >= 0x88 && opcode <= 0x8E) {
            original_code_size += 2; // mov [rXX], rXX
        }
        else if (opcode == 0x90) {
            original_code_size += 1; // nop
        }
        else if (opcode == 0x68) {
            original_code_size += 5; // push imm32
        }
        else if (opcode == 0xE9 || opcode == 0xEB) {
            original_code_size += (opcode == 0xE9) ? 5 : 2; // jmp rel32 / jmp rel8
        }
        else if (opcode == 0xFF && code[original_code_size + 1] == 0x25) {
            original_code_size += 6; // jmp [rip+imm32]
        }
        else if (opcode == 0x48 && code[original_code_size + 1] == 0x83 && code[original_code_size + 2] == 0xEC) {
            original_code_size += 4; // sub rsp, imm8
        }
        else if (opcode == 0x48 && code[original_code_size + 1] == 0x81 && code[original_code_size + 2] == 0xEC) {
            original_code_size += 7; // sub rsp, imm32
        }
        else {
            PrintError(L"GetOriginalCodeSize: Unrecognized opcode encountered: {:#x} at address: {:#x}", opcode, reinterpret_cast<std::size_t>(&code[original_code_size]));
            return 0;
        }
    }
    return original_code_size;
}

std::uint8_t* Hooking::GenerateImmediateJump(std::uint8_t* code, std::uint8_t* jump_value)
{
    std::uintptr_t jump_source = jump_value - (code + 5); // error ?
    *code++ = 0xE9;  // jmp +imm32
    *reinterpret_cast<std::int32_t*>(code) = static_cast<std::int32_t>(jump_source);
    code += sizeof(std::int32_t);
    return code;
}

//std::uint8_t* Hooking::GenerateIndirectJump(std::uint8_t* code, std::uint8_t** jump_value)
//{
//    std::uintptr_t jump_source = *jump_value - (code + 6);
//    *code++ = 0xFF;   // jmp [+imm32]
//    *code++ = 0x25;
//    *reinterpret_cast<std::int32_t*>(code) = static_cast<std::int32_t>(jump_source);
//    code += sizeof(std::int32_t);
//    return code;
//}

std::uint8_t* Hooking::GenerateIndirectJump(std::uint8_t* code, std::uint8_t** jump_value)
{
    *code++ = 0x48; // rex.w
    *code++ = 0xB8; // mov rax, imm64
    *reinterpret_cast<std::uint64_t*>(code) = reinterpret_cast<std::uint64_t>(*jump_value);
    code += sizeof(std::uint64_t);
    *code++ = 0xFF; // jmp rax
    *code++ = 0xE0;
    return code;
}

//std::uint8_t* Hooking::GenerateIndirectJump(std::uint8_t* code, std::uint8_t** jump_value)
//{
//    *code++ = 0x50; // push rax
//    *code++ = 0x48; // rex.w
//    *code++ = 0xB8; // mov rax, imm64
//    *reinterpret_cast<std::uint64_t*>(code) = reinterpret_cast<std::uint64_t>(*jump_value);
//    code += sizeof(std::uint64_t);
//    *code++ = 0xFF; // jmp rax
//    **jump_value = 0x58; // pop rax
//    return code;
//}

std::uint8_t* Hooking::GenerateBreakpoint(std::uint8_t* code, std::uint8_t* limit)
{
    while (code < limit) {
        *code++ = 0xCC; // brk;
    }
    return code;
}

std::vector<Hooking::HookData> Hooking::hook_data_list;
#endif