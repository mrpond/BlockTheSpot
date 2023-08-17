#ifndef _HOOKING_H
#define _HOOKING_H

#include <Windows.h>
#pragma warning(disable: 4530)
#include <vector>
#pragma warning(default: 4530)

class Hooking {
public:
    static bool HookFunction(PVOID* ppFunctionPointer, PVOID pHookFunction);
    static bool UnhookFunction(PVOID* ppFunctionPointer, PVOID pHookFunction = nullptr);

private:
#ifndef ENABLE_DETOURS
    struct HookData {
        PVOID* ppFunctionPointer;
        PBYTE pbCode;
        PBYTE pbNewCode;
        std::size_t originalCodeSize;
        std::uint8_t g_OriginalCode[5];
    };

    static std::vector<HookData> HookDataList;

    static bool BackupFunctionCode(HookData& hookData);
    static bool ApplyHook(HookData& hookData, PVOID pHookFunction);
    static bool RestoreFunctionCode(HookData& hookData);
    static std::size_t GetOriginalCodeSize(PBYTE pbCode);
#endif
};

#endif //_HOOKING_H
