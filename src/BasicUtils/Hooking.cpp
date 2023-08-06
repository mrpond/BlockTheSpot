#include "Hooking.h"
#include <stdexcept>
#include <mutex>
#include <detours.h>

std::mutex mtx;

bool Hooking::HookFunction(HookData data) 
{
    std::scoped_lock lock(mtx);
    if (Begin()) {
        for (auto& [ppPointer, pDetour] : data) {
            if (FindHookFunction(ppPointer) != nullptr) {
                throw std::runtime_error("DetourAttach failed");
            }

            if (DetourAttach(ppPointer, pDetour) != NO_ERROR) {
                throw std::runtime_error("DetourAttach failed");
            }

            InsertHookFunction(ppPointer, pDetour);
        }
        return Commit();
    }
    return false;
}

bool Hooking::HookFunction(PVOID* ppPointers, PVOID pDetours)
{
    HookData data = { {ppPointers, pDetours} };
    return HookFunction(data);
}

bool Hooking::UnhookFunction(HookData data) 
{
    std::scoped_lock lock(mtx);
    if (Begin()) {
        for (auto& [ppPointer, pDetour] : data) {
            if (pDetour == nullptr) {
                pDetour = FindHookFunction(ppPointer);
                if (pDetour == nullptr) {
                    throw std::runtime_error("DetourDetach failed");
                }
            }

            if (DetourDetach(ppPointer, pDetour) != NO_ERROR) {
                throw std::runtime_error("DetourDetach failed");
            }

            EraseHookFunction(ppPointer);
        }
        return Commit();
    }
    return false;
}

bool Hooking::UnhookFunction(PVOID* ppPointers, PVOID pDetours) 
{
    HookData data = { {ppPointers, pDetours} };
    return UnhookFunction(data);
}

bool Hooking::Begin()
{
    if (DetourIsHelperProcess()) {
        throw std::runtime_error("DetourIsHelperProcess failed");
    }

    DetourRestoreAfterWith();

    if (DetourTransactionBegin() != NO_ERROR) {
        throw std::runtime_error("DetourTransactionBegin failed");
    }

    return true;
}

bool Hooking::Commit() 
{
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

PVOID Hooking::FindHookFunction(PVOID* ppPointers)
{
    auto iter = HookFunctionList.find(ppPointers);
    if (iter != HookFunctionList.end()) {
        return iter->second;
    }
    return nullptr;
}

Hooking::HookData Hooking::HookFunctionList;