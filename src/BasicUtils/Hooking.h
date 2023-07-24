#ifndef _HOOKING_H
#define _HOOKING_H

#include <Windows.h>
#include <vector>
#include <map>

class Hooking {
public:
    static bool HookFunction(std::vector<PVOID*> ppPointers, std::vector<PVOID> pDetours);
    static bool HookFunction(PVOID* ppPointers, PVOID pDetours);
    static bool UnhookFunction(std::vector<PVOID*> ppPointers, std::vector<PVOID> pDetours = {});
    static bool UnhookFunction(PVOID* ppPointers, PVOID pDetours = nullptr);


private:
    static bool Begin();
    static bool Commit();
    static void InsertHookFunction(PVOID* ppPointers, PVOID pDetours);
    static void EraseHookFunction(PVOID* ppPointers);
    static PVOID FindHookFunction(PVOID* ppPointers);
    static std::map<PVOID*, PVOID> HookFunctionList;
};

#endif //_HOOKING_H
