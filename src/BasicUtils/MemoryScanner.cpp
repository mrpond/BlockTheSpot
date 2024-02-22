#include "MemoryScanner.h"
#include "Hooking.h"
#include "Utils.h"
#include "Memory.h"
#include <sstream>
#include <Psapi.h>
#include <algorithm>
#include <Memory.h>
#include <unordered_map>

namespace MemoryScanner 
{
    ModuleInfo GetModuleInfo(std::wstring_view module_name)
    {
        static std::unordered_map<std::wstring_view, ModuleInfo> loaded_modules;

        const auto module = loaded_modules.find(module_name);
        if (module != loaded_modules.end()) {
            return module->second;
        }

        HMODULE module_handle = GetModuleHandleW(module_name.empty() ? nullptr : module_name.data());
        if (module_handle == nullptr) {
            return ModuleInfo(module_name, 0, 0);
        }

        MODULEINFO module_info;
        if (!GetModuleInformation(GetCurrentProcess(), module_handle, &module_info, sizeof(MODULEINFO))) {
            return ModuleInfo(module_name, 0, 0);
        }

        const auto ret = ModuleInfo(module_name, reinterpret_cast<uintptr_t>(module_info.lpBaseOfDll), module_info.SizeOfImage);
        loaded_modules.emplace(module_name, ret);
        return ret;
    }

    ScanResult GetFunctionAddress(std::string_view module_name, std::string_view function_name)
    {
        HMODULE module_handle = GetModuleHandleA(module_name.data());
        if (module_handle == nullptr) {
            module_handle = LoadLibraryA(module_name.data());
            if (module_handle == nullptr) {
                return ScanResult(0, 0, 0);
            }
        }

        FARPROC function_address = GetProcAddress(module_handle, function_name.data());
        if (function_address == nullptr) {
            return ScanResult(0, 0, 0);
        }

        MODULEINFO module_info;
        if (!GetModuleInformation(GetCurrentProcess(), module_handle, &module_info, sizeof(MODULEINFO))) {
            return ScanResult(reinterpret_cast<uintptr_t>(function_address), 0, 0);
        }

        return ScanResult(reinterpret_cast<uintptr_t>(function_address), reinterpret_cast<uintptr_t>(module_info.lpBaseOfDll), module_info.SizeOfImage);
    }

    std::vector<uint8_t> SignatureToByteArray(std::wstring_view signature)
    {
        std::vector<uint8_t> signature_bytes;
        std::wstring word;
        std::wistringstream iss(signature.data());

        while (iss >> word) {
            if (word.size() == 1 && word[0] == L'?') {
                signature_bytes.push_back(0);
            }
            else if (word.size() == 2 && word[0] == L'?' && word[1] == L'?') {
                signature_bytes.push_back(0);
            }
            else if (word.size() == 2 && std::isxdigit(word[0]) && std::isxdigit(word[1])) {
                unsigned long value = std::stoul(word, nullptr, 16);
                if (value > 255) {
                    return { 0 };
                }
                signature_bytes.push_back(static_cast<uint8_t>(value));
            }
            else {
                for (wchar_t c : word) {
                    if (c > 255) {
                        return { 0 };
                    }
                    signature_bytes.push_back(static_cast<uint8_t>(c));
                }
            }
        }

        return signature_bytes;
    }

    std::vector<ScanResult> ScanAll(uintptr_t base_address, size_t image_size, const std::vector<uint8_t>& pattern_byte, bool only_first)
    {
        std::vector<ScanResult> matches;

        size_t pattern_size = pattern_byte.size();

        if (pattern_byte.empty()) {
            return matches;
        }

        uint8_t* base_ptr = reinterpret_cast<uint8_t*>(base_address);
        uintptr_t end_address = base_address + image_size - pattern_size + 1;

        while (base_ptr < reinterpret_cast<uint8_t*>(end_address)) {
            base_ptr = static_cast<uint8_t*>(memchr(base_ptr, pattern_byte[0], end_address - reinterpret_cast<uintptr_t>(base_ptr)));
            if (!base_ptr) {
                break;
            }

            bool found = true;
            for (size_t i = 1; i < pattern_size; ++i) {
                if (pattern_byte[i] != 0 && pattern_byte[i] != base_ptr[i]) {
                    found = false;
                    break;
                }
            }

            if (found) {
                matches.push_back(ScanResult(reinterpret_cast<uintptr_t>(base_ptr), base_address, image_size));
                if (only_first) break;
            }

            ++base_ptr;
        }

        return matches;
    }

    std::vector<ScanResult> ScanAll(uintptr_t base_address, size_t image_size, std::wstring_view signature)
    {
        return ScanAll(base_address, image_size, SignatureToByteArray(signature));
    }

    std::vector<ScanResult> ScanAll(std::wstring_view signature, std::wstring_view module_name)
    {
        const auto mod = GetModuleInfo(module_name);
        return ScanAll(mod.base_address, mod.module_size, signature);
    }

    ScanResult ScanFirst(uintptr_t base_address, size_t image_size, const std::vector<uint8_t>& pattern_byte)
    {
        const auto matches = ScanAll(base_address, image_size, pattern_byte, true);
        return matches.empty() ? ScanResult(0, 0, 0) : matches.at(0);
    }

    ScanResult ScanFirst(uintptr_t base_address, size_t image_size, std::wstring_view signature)
    {
        return ScanFirst(base_address, image_size, SignatureToByteArray(signature));
    }

    ScanResult ScanFirst(std::wstring_view signature, std::wstring_view module_name)
    {
        const auto mod = GetModuleInfo(module_name);
        return ScanFirst(mod.base_address, mod.module_size, signature);
    }

    ScanResult::ScanResult(uintptr_t address, uintptr_t base, size_t size) : m_address(address), m_base_address(base), m_image_size(size)
    {
        //...
    }

    ScanResult::operator uintptr_t() const
    {
        return m_address;
    }

    bool ScanResult::is_valid(const std::vector<uint8_t>& value) const
    {
        if (m_address == 0) {
            return false;
        }

        for (size_t i = 0; i < value.size(); ++i) {
            if (*(reinterpret_cast<uint8_t*>(m_address) + i) != value[i])
                return false;
        }

        return true;
    }

    uint8_t* ScanResult::data() const
    {
        if (!is_valid()) {
            return nullptr;
        }

        return reinterpret_cast<uint8_t*>(m_address);
    }

    ScanResult ScanResult::rva() const
    {
        if (!is_valid()) {
            return ScanResult(0, m_base_address, m_image_size);
        }

        uintptr_t rva_address = m_address - m_base_address;
        return ScanResult(rva_address, m_base_address, m_image_size);
    }

    ScanResult ScanResult::offset(std::ptrdiff_t offset_value) const
    {
        if (!is_valid()) {
            return ScanResult(0, m_base_address, m_image_size);
        }

        uintptr_t new_address = m_address;
        if (offset_value >= 0) {
            new_address += static_cast<uintptr_t>(offset_value);
        }
        else {
            new_address -= static_cast<uintptr_t>(-offset_value);
        }

        return ScanResult(new_address, m_base_address, m_image_size);
    }

    ScanResult ScanResult::scan_first(std::wstring_view value) const
    {
        return is_valid() ? ScanFirst(m_address, m_image_size - rva(), value) : ScanResult(0, m_base_address, m_image_size);
    }

    bool ScanResult::write(const void* data, size_t size) const
    {
        return Memory::Write(reinterpret_cast<void*>(m_address), data, size);
    }

    bool ScanResult::write(std::string_view data) const
    {
        return Memory::Write(reinterpret_cast<void*>(m_address), data);
    }

    bool ScanResult::write(std::initializer_list<uint8_t> data) const
    {
        return Memory::Write(reinterpret_cast<void*>(m_address), data);
    }

    PVOID* ScanResult::hook(PVOID hook_function) const
    {
        return (is_valid() && Hooking::HookFunction(&(PVOID&)m_address, hook_function)) ? reinterpret_cast<PVOID*>(m_address) : NULL;
    }

    bool ScanResult::unhook() const
    {
        return is_valid() ? Hooking::UnhookFunction(&(PVOID&)m_address) : false;
    }

    std::vector<ScanResult> ScanResult::get_all_matching_codes(const std::vector<uint8_t>& pattern_byte, bool calculate_relative_offset, uintptr_t base_address, size_t image_size, bool only_first) const
    {
        if (base_address == 0) base_address = m_base_address;
        if (image_size == 0) image_size = m_image_size;

        if (!calculate_relative_offset) {
            std::vector<uint8_t> new_pattern_byte = pattern_byte;
            new_pattern_byte.insert(new_pattern_byte.end(), reinterpret_cast<const uint8_t*>(&m_address),
                reinterpret_cast<const uint8_t*>(&m_address) + sizeof(m_address));

            return ScanAll(base_address, image_size, new_pattern_byte, only_first);
        }
        
        std::vector<ScanResult> matches;
        const auto all_matches = ScanAll(base_address, image_size, pattern_byte);
        for (const auto& address : all_matches) {
            const auto offset_address = address + pattern_byte.size();
            const auto relative_offset = static_cast<int32_t>(m_address) - static_cast<int32_t>(offset_address) - sizeof(int32_t);
            if (*reinterpret_cast<const int32_t*>(offset_address) == relative_offset) {
                matches.push_back(ScanResult(address, base_address, image_size));
                if (only_first) break;
            }
        }

        return matches;
    }

    ScanResult ScanResult::get_first_matching_code(const std::vector<uint8_t>& pattern_byte, bool calculate_relative_offset, uintptr_t base_address, size_t image_size) const
    {
        const auto matches = get_all_matching_codes(pattern_byte, calculate_relative_offset, base_address, image_size, true);
        return matches.empty() ? ScanResult(0, m_base_address, m_image_size) : matches.at(0);
    }

    uintptr_t ScanResult::get_base_address() const
    {
        return m_base_address;
    }

    size_t ScanResult::get_image_size() const
    {
        return m_image_size;
    }

    void ScanResult::print_address() const
    {
        Print(L"{:x}", m_address);
    }
}