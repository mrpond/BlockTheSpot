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

bool Hooking::HookFunction(PVOID* ppFunctionPointer, PVOID pHookFunction)
{
    std::lock_guard<std::mutex> lock(mtx);

    if (!ppFunctionPointer || !pHookFunction) {
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

    if ((error = DetourAttach(ppFunctionPointer, pHookFunction)) != NO_ERROR) {
        PrintError(L"DetourAttach error: {}", error);
        return false;
    }

    if ((error = DetourTransactionCommit()) != NO_ERROR) {
        PrintError(L"DetourTransactionCommit error: {}", error);
        return false;
    }
#else // It is currently in the testing phase. There may be errors.
    if (!ppFunctionPointer || !pHookFunction) {
        PrintError(L"HookFunction: Invalid function pointer or hook function.");
        return false;
    }

    auto hookDataIt = std::find_if(HookDataList.begin(), HookDataList.end(),
        [ppFunctionPointer](const HookData& hookData) {
            return hookData.ppFunctionPointer == ppFunctionPointer;
        });

    if (hookDataIt != HookDataList.end()) {
        PrintError(L"HookFunction: Function pointer is already hooked.");
        return false;
    }

    HookData hookData;
    hookData.ppFunctionPointer = ppFunctionPointer;
    hookData.pbCode = reinterpret_cast<PBYTE>(*ppFunctionPointer);

    if (!BackupFunctionCode(hookData)) {
        PrintError(L"HookFunction: BackupFunctionCode failed.");
        return false;
    }

    if (!ApplyHook(hookData, pHookFunction)) {
        PrintError(L"HookFunction: ApplyHook failed.");
        return false;
    }

    HookDataList.push_back(hookData);

    *ppFunctionPointer = hookData.pbNewCode;
#endif
    return true;
}

bool Hooking::UnhookFunction(PVOID* ppFunctionPointer, PVOID pHookFunction)
{
    std::lock_guard<std::mutex> lock(mtx);
#ifdef ENABLE_DETOURS
    if (!ppFunctionPointer || !pHookFunction) {
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

    if ((error = DetourDetach(ppFunctionPointer, pHookFunction)) != NO_ERROR) {
        PrintError(L"DetourDetach error: {}", error);
        return false;
    }

    if ((error = DetourTransactionCommit()) != NO_ERROR) {
        PrintError(L"DetourTransactionCommit error: {}", error);
        return false;
    }
#else
    if (!ppFunctionPointer) {
        PrintError(L"UnhookFunction: Invalid function pointer.");
        return false;
    }

    auto hookDataIt = std::find_if(HookDataList.begin(), HookDataList.end(),
        [ppFunctionPointer](const HookData& hookData) {
            return hookData.ppFunctionPointer == ppFunctionPointer;
        });

    if (hookDataIt == HookDataList.end()) {
        PrintError(L"UnhookFunction: Function pointer is not hooked.");
        return false;
    }

    if (!RestoreFunctionCode(*hookDataIt)) {
        PrintError(L"UnhookFunction: RestoreFunctionCode failed.");
        return false;
    }

    HookDataList.erase(hookDataIt);
#endif
    return true;
}

#ifndef ENABLE_DETOURS
bool Hooking::BackupFunctionCode(HookData& hookData)
{
    hookData.originalCodeSize = GetOriginalCodeSize(hookData.pbCode);
    if (hookData.originalCodeSize < 5) {
        PrintError(L"BackupFunctionCode: Original code size is less than 5 bytes.");
        return false;
    }

    hookData.pbNewCode = reinterpret_cast<PBYTE>(VirtualAlloc(nullptr, hookData.originalCodeSize + 14, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!hookData.pbNewCode) {
        PrintError(L"BackupFunctionCode: Failed to allocate memory for new code.");
        return false;
    }

    memcpy(hookData.pbNewCode, hookData.pbCode, hookData.originalCodeSize);

#ifdef _WIN64
    hookData.pbNewCode[hookData.originalCodeSize] = 0xFF;
    hookData.pbNewCode[hookData.originalCodeSize + 1] = 0x25;
    PVOID pOffset = hookData.pbCode + hookData.originalCodeSize;
    memcpy(hookData.pbNewCode + hookData.originalCodeSize + 6, &pOffset, sizeof(pOffset));
#else
    hookData.pbNewCode[hookData.originalCodeSize] = 0xE9;
    DWORD dwOffset = (DWORD)(hookData.pbCode - hookData.pbNewCode - 5);
    memcpy(hookData.pbNewCode + hookData.originalCodeSize + 1, &dwOffset, sizeof(dwOffset));
#endif

    return true;
}

bool Hooking::ApplyHook(HookData& hookData, PVOID pHookFunction)
{
    DWORD oldProtect;
    if (!VirtualProtect(hookData.pbCode, hookData.originalCodeSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        PrintError(L"ApplyHook: Failed to change memory protection for code.");
        return false;
    }

    memcpy(hookData.g_OriginalCode, hookData.pbCode, hookData.originalCodeSize);

    INT32 offset = INT32(reinterpret_cast<PBYTE>(pHookFunction) - hookData.pbCode - 5);

    if (offset < 0) {
        PrintError(L"ApplyHook: Invalid offset.");
        return false;
    }

    hookData.pbCode[0] = 0xE9;
    memcpy(hookData.pbCode + 1, &offset, sizeof(offset));

    memset(hookData.pbCode + 5, 0xCC, hookData.originalCodeSize - 5);

    DWORD newProtect;
    if (!VirtualProtect(hookData.pbCode, hookData.originalCodeSize, oldProtect, &newProtect)) {
        PrintError(L"ApplyHook: Failed to restore memory protection for code.");
        return false;
    }

    return true;
}


bool Hooking::RestoreFunctionCode(HookData& hookData)
{
    DWORD oldProtect;
    if (!VirtualProtect(hookData.pbCode, hookData.originalCodeSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        PrintError(L"RestoreFunctionCode: Failed to change memory protection for code.");
        return false;
    }

    memcpy(hookData.pbCode, hookData.g_OriginalCode, hookData.originalCodeSize);

    DWORD newProtect;
    if (!VirtualProtect(hookData.pbCode, hookData.originalCodeSize, oldProtect, &newProtect)) {
        PrintError(L"RestoreFunctionCode: Failed to restore memory protection for code.");
        return false;
    }

    if (!VirtualFree(hookData.pbNewCode, 0, MEM_RELEASE)) {
        PrintError(L"RestoreFunctionCode: Failed to free memory allocated for new code.");
        return false;
    }

    return true;
}

std::size_t Hooking::GetOriginalCodeSize(PBYTE pbCode)
{
    std::size_t originalCodeSize = 0;

    while (originalCodeSize < 5) {
        std::uint8_t opcode = pbCode[originalCodeSize];

        if (opcode >= 0x50 && opcode <= 0x57) {
            originalCodeSize += 1; // push rXX
        }
        else if (opcode == 0x41 && pbCode[originalCodeSize + 1] >= 0x50 && pbCode[originalCodeSize + 1] <= 0x57) {
            originalCodeSize += 2; // push rXX
        }
        else if (opcode >= 0x88 && opcode <= 0x8E) {
            originalCodeSize += 2; // mov [rXX], rXX
        }
        else if (opcode == 0x90) {
            originalCodeSize += 1; // nop
        }
        else if (opcode == 0x68) {
            originalCodeSize += 5; // push imm32
        }
        else if (opcode == 0xE9 || opcode == 0xEB) {
            originalCodeSize += (opcode == 0xE9) ? 5 : 2; // jmp rel32 / jmp rel8
        }
        else if (opcode == 0xFF && pbCode[originalCodeSize + 1] == 0x25) {
            originalCodeSize += 6; // jmp [rip+imm32]
        }
        else if (opcode == 0x48 && pbCode[originalCodeSize + 1] == 0x83 && pbCode[originalCodeSize + 2] == 0xEC) {
            originalCodeSize += 4; // sub rsp, imm8
        }
        else if (opcode == 0x48 && pbCode[originalCodeSize + 1] == 0x81 && pbCode[originalCodeSize + 2] == 0xEC) {
            originalCodeSize += 7; // sub rsp, imm32
        }
        else {
            PrintError(L"GetOriginalCodeSize: Unrecognized opcode encountered: {:#x} at address: {:#x}", opcode, reinterpret_cast<std::size_t>(&pbCode[originalCodeSize]));
            originalCodeSize = 0;
            break;
        }
    }

    return originalCodeSize;
}

std::vector<Hooking::HookData> Hooking::HookDataList;
#endif
