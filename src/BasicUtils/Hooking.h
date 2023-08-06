#ifndef _HOOKING_H
#define _HOOKING_H

#include <Windows.h>
#include <vector>
#include <map>

class Hooking {
public:
    using HookData = std::map<PVOID*, PVOID>;
    static bool HookFunction(HookData data);
    static bool HookFunction(PVOID* ppPointers, PVOID pDetours);
    static bool UnhookFunction(HookData data);
    static bool UnhookFunction(PVOID* ppPointers, PVOID pDetours = nullptr);


private:
    static bool Begin();
    static bool Commit();
    static void InsertHookFunction(PVOID* ppPointers, PVOID pDetours);
    static void EraseHookFunction(PVOID* ppPointers);
    static PVOID FindHookFunction(PVOID* ppPointers);
    static HookData HookFunctionList;
};

#endif //_HOOKING_H
