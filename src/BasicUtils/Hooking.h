#ifndef _HOOKING_H
#define _HOOKING_H

#include <Windows.h>
#pragma warning(disable: 4530)
#include <vector>
#pragma warning(default: 4530)

class Hooking {
public:
    static bool HookFunction(void** function_pointer, void* hook_function);
    static bool UnhookFunction(void** function_pointer, void* hook_function = nullptr);

private:
#ifndef ENABLE_DETOURS
    struct HookData {
        void** function_pointer;
        std::uint8_t* code;
        std::uint8_t* new_code;
        std::vector<std::uint8_t> original_code;
    };

    static std::vector<HookData> hook_data_list;

    static bool BackupFunctionCode(HookData& hook_data);
    static bool ApplyHook(HookData& hook_data, void* hook_function);
    static bool RestoreFunctionCode(HookData& hook_data);
    static std::size_t GetOriginalCodeSize(std::uint8_t* code);

    static std::uint8_t* GenerateImmediateJump(std::uint8_t* code, std::uint8_t* jump_value);
    static std::uint8_t* GenerateIndirectJump(std::uint8_t* code, std::uint8_t** jump_value);
    static std::uint8_t* GenerateBreakpoint(std::uint8_t* code, std::uint8_t* limit);
#endif
};

#endif //_HOOKING_H
