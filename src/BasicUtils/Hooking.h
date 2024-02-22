#ifndef _HOOKING_H
#define _HOOKING_H

#include <Windows.h>

namespace Hooking {
    bool HookFunction(PVOID* function_pointer, PVOID hook_function);
    bool UnhookFunction(PVOID* function_pointer, PVOID hook_function = nullptr);
}

#endif //_HOOKING_H
