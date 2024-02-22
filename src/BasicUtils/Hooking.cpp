#include "Hooking.h"

#include <stdexcept>
#include <mutex>
#include <unordered_map>
#include <detours.h>

namespace Hooking {
    std::mutex mtx;
    std::unordered_map<PVOID, PVOID> hooked_functions;

    bool HookFunction(PVOID* function_pointer, PVOID hook_function) {
        std::lock_guard<std::mutex> lock(mtx);

        if (!function_pointer || !hook_function) {
            throw std::invalid_argument("Invalid function pointer or hook function.");
        }

        if (hooked_functions.find(*function_pointer) != hooked_functions.end()) {
            return false; // Function is already hooked
        }

        LONG error = DetourTransactionBegin();
        if (error != NO_ERROR) {
            throw std::runtime_error("DetourTransactionBegin error: " + std::to_string(error));
        }

        error = DetourUpdateThread(GetCurrentThread());
        if (error != NO_ERROR) {
            DetourTransactionAbort();
            throw std::runtime_error("DetourUpdateThread error: " + std::to_string(error));
        }

        error = DetourAttach(function_pointer, hook_function);
        if (error != NO_ERROR) {
            DetourTransactionAbort();
            throw std::runtime_error("DetourAttach error: " + std::to_string(error));
        }

        error = DetourTransactionCommit();
        if (error != NO_ERROR) {
            DetourTransactionAbort();
            throw std::runtime_error("DetourTransactionCommit error: " + std::to_string(error));
        }

        hooked_functions[*function_pointer] = hook_function;

        return true;
    }

    bool UnhookFunction(PVOID* function_pointer, PVOID hook_function) {
        std::lock_guard<std::mutex> lock(mtx);

        if (!function_pointer) {
            throw std::invalid_argument("Invalid function pointer.");
        }

        const auto it = hooked_functions.find(*function_pointer);
        if (it == hooked_functions.end()) {
            return false; // Function is not hooked
        }

        PVOID actual_hook_function = hook_function ? hook_function : it->second;

        LONG error = DetourTransactionBegin();
        if (error != NO_ERROR) {
            throw std::runtime_error("DetourTransactionBegin error: " + std::to_string(error));
        }

        error = DetourUpdateThread(GetCurrentThread());
        if (error != NO_ERROR) {
            DetourTransactionAbort();
            throw std::runtime_error("DetourUpdateThread error: " + std::to_string(error));
        }

        error = DetourDetach(function_pointer, actual_hook_function);
        if (error != NO_ERROR) {
            DetourTransactionAbort();
            throw std::runtime_error("DetourDetach error: " + std::to_string(error));
        }

        error = DetourTransactionCommit();
        if (error != NO_ERROR) {
            DetourTransactionAbort();
            throw std::runtime_error("DetourTransactionCommit error: " + std::to_string(error));
        }

        hooked_functions.erase(it);

        return true;
    }
}
