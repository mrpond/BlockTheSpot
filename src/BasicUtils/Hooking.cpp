#include "Hooking.h"
#include <stdexcept>
#include "../detours/detours.h"

#pragma comment(lib, "Detours.lib")

bool Hooking::HookFunction(std::vector<PVOID*> ppPointers, std::vector<PVOID> pDetours) {
    if (Begin())
    {
        for (size_t i = 0; i < ppPointers.size(); i++) {
            if (FindHookFunction(ppPointers[i]) != nullptr) {
                throw std::runtime_error("DetourAttach failed");
            }

            if (DetourAttach(ppPointers[i], pDetours[i]) != NO_ERROR) {
                throw std::runtime_error("DetourAttach failed");
            }

            InsertHookFunction(ppPointers[i], pDetours[i]);
        }
        return Commit();
    }
    return false;
}

bool Hooking::HookFunction(PVOID* ppPointers, PVOID pDetours) {
    auto _ppPointers = { ppPointers };
    auto _pDetours = { pDetours };
    return HookFunction(_ppPointers, _pDetours);
}

bool Hooking::UnhookFunction(std::vector<PVOID*> ppPointers, std::vector<PVOID> pDetours) {
    if (Begin())
    {
        for (size_t i = 0; i < ppPointers.size(); i++) {
            if (pDetours[i] == nullptr) {
                pDetours[i] = FindHookFunction(ppPointers[i]);
                if (pDetours[i] == nullptr) {
                    throw std::runtime_error("DetourDetach failed");
                }
            }

            if (DetourDetach(ppPointers[i], pDetours[i]) != NO_ERROR) {
                throw std::runtime_error("DetourDetach failed");
            }

            EraseHookFunction(ppPointers[i]);
        }
        return Commit();
    }
    return false;
}

bool Hooking::UnhookFunction(PVOID* ppPointers, PVOID pDetours) {
    auto _ppPointers = { ppPointers };
    auto _pDetours = { pDetours };
    return UnhookFunction(_ppPointers, _pDetours);
}

bool Hooking::Begin() {
    if (DetourIsHelperProcess()) {
        throw std::runtime_error("DetourIsHelperProcess failed");
    }

    DetourRestoreAfterWith();

    if (DetourTransactionBegin() != NO_ERROR) {
        throw std::runtime_error("DetourTransactionBegin failed");
    }

    return true;
}

bool Hooking::Commit() {
    if (DetourTransactionCommit() != NO_ERROR) {
        throw std::runtime_error("DetourTransactionCommit failed");
    }
    return true;
}

void Hooking::InsertHookFunction(PVOID* ppPointers, PVOID pDetours)
{
    HookFunctionList[ppPointers] = pDetours;
}

void Hooking::EraseHookFunction(PVOID* ppPointers)
{
    HookFunctionList.erase(ppPointers);
}

PVOID Hooking::FindHookFunction(PVOID* ppPointers) {
    auto iter = HookFunctionList.find(ppPointers);
    if (iter != HookFunctionList.end()) {
        return iter->second;
    }
    return nullptr;
}

std::map<PVOID*, PVOID> Hooking::HookFunctionList;