#include "MemoryScanner.h"
#include "Hooking.h"
#include "Utils.h"
#include "Memory.h"
#include <sstream>
#include <Psapi.h>
#include <algorithm>
#include <unordered_map>
#include <execution>

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

    std::vector<BytePattern> ParseBytePattern(std::wstring_view byte_pattern)
    {
        std::vector<BytePattern> parsed_pattern;
        bool is_hex = byte_pattern.find_first_not_of(L"0123456789ABCDEFabcdef? ") == std::wstring::npos;
        if (is_hex) {
            std::wistringstream iss(byte_pattern.data());
            std::wstring byte;
            while (iss >> byte) {
                BytePattern bp;
                if (byte.size() == 1 && byte[0] == L'?') {
                    bp.half_byte[0].wildcard = true;
                    bp.half_byte[1].wildcard = true;
                }
                else {
                    if (byte[0] == L'?') {
                        bp.half_byte[0].wildcard = true;
                    }
                    else {
                        bp.half_byte[0].data = std::stoi(std::wstring(1, byte[0]), nullptr, 16);
                    }

                    if (byte[1] == L'?') {
                        bp.half_byte[1].wildcard = true;
                    }
                    else {
                        bp.half_byte[1].data = std::stoi(std::wstring(1, byte[1]), nullptr, 16);
                    }
                }
                parsed_pattern.push_back(bp);
            }
        }
        else {
            for (wchar_t ch : byte_pattern) {
                BytePattern bp;
                bp.half_byte[0].data = ch >> 4;
                bp.half_byte[1].data = ch & 0xF;
                parsed_pattern.push_back(bp);
            }
        }

        return parsed_pattern;
    }

    std::vector<ScanResult> ScanAll(uintptr_t base_address, size_t image_size, const std::vector<BytePattern>& parsed_pattern, bool only_first)
    {
        std::vector<ScanResult> result;
        const uint8_t* start = reinterpret_cast<const uint8_t*>(base_address);
        const uint8_t* end = start + image_size;
        const size_t pattern_size = parsed_pattern.size();
        const BytePattern& first_pattern = parsed_pattern[0];

        for (const uint8_t* it = start; it + pattern_size <= end; ++it) {
            if ((first_pattern.half_byte[0].wildcard || ((it[0] & 0xF0) == (first_pattern.half_byte[0].data << 4))) &&
                (first_pattern.half_byte[1].wildcard || ((it[0] & 0x0F) == first_pattern.half_byte[1].data))) {
                
                bool found = true;
                for (size_t i = 1; i < pattern_size; ++i) {
                    const BytePattern& pattern = parsed_pattern[i];
                    uint8_t byte = it[i];

                    if (!(pattern.half_byte[0].wildcard || ((byte & 0xF0) == (pattern.half_byte[0].data << 4))) ||
                        !(pattern.half_byte[1].wildcard || ((byte & 0x0F) == pattern.half_byte[1].data))) {
                        found = false;
                        break;
                    }
                }

                if (found) {
                    result.push_back(ScanResult(reinterpret_cast<uintptr_t>(it), base_address, image_size));
                    if (only_first) {
                        break;
                    }
                }
            }
        }

        return result;
    }

    std::vector<ScanResult> ScanAll(uintptr_t base_address, size_t image_size, std::wstring_view pattern)
    {
        return ScanAll(base_address, image_size, ParseBytePattern(pattern));
    }

    std::vector<ScanResult> ScanAll(std::wstring_view pattern, std::wstring_view module_name)
    {
        const auto module_info = GetModuleInfo(module_name);
        return ScanAll(module_info.base_address, module_info.module_size, pattern);
    }

    ScanResult ScanFirst(uintptr_t base_address, size_t image_size, const std::vector<BytePattern>& parsed_pattern)
    {
        const auto addresses = ScanAll(base_address, image_size, parsed_pattern, true);
        return addresses.empty() ? ScanResult(0, 0, 0) : addresses.front();
    }

    ScanResult ScanFirst(uintptr_t base_address, size_t image_size, std::wstring_view pattern)
    {
        return ScanFirst(base_address, image_size, ParseBytePattern(pattern));
    }

    ScanResult ScanFirst(std::wstring_view pattern, std::wstring_view module_name)
    {
        const auto module_info = GetModuleInfo(module_name);
        return ScanFirst(module_info.base_address, module_info.module_size, pattern);
    }

    ScanResult::ScanResult(uintptr_t address, uintptr_t base, size_t size, bool is_rva) : m_address(address), m_base_address(base), m_image_size(size)
    {
        if (is_rva && address) {
            m_address += m_base_address;
        }
    }
    
    ScanResult::ScanResult(uintptr_t address, std::wstring_view module_name, bool is_rva) : m_address(address), m_base_address(GetModuleInfo(module_name).base_address), m_image_size(GetModuleInfo(module_name).module_size)
    {
        if (is_rva && address) {
            m_address += m_base_address;
        }
    }

    ScanResult::operator uintptr_t() const
    {
        return m_address;
    }

    bool ScanResult::is_valid(const std::vector<BytePattern>& parsed_pattern) const
    {
        if (m_address == 0) {
            return false;
        }

        for (size_t i = 0; i < parsed_pattern.size(); ++i) {
            BytePattern pattern = parsed_pattern[i];
            if (pattern.half_byte[0].wildcard && pattern.half_byte[1].wildcard) {
                continue;
            }
            
            uint8_t byte = *(reinterpret_cast<uint8_t*>(m_address) + i);
            uint8_t pattern_byte = (pattern.half_byte[0].data << 4) | pattern.half_byte[1].data;
            bool match_high_nibble = pattern.half_byte[0].wildcard || (byte & 0xF0) == (pattern_byte & 0xF0);
            bool match_low_nibble = pattern.half_byte[1].wildcard || (byte & 0x0F) == (pattern_byte & 0x0F);

            if (!(match_high_nibble && match_low_nibble)) {
                return false;
            }
        }

        return true;
    }

    bool ScanResult::is_valid(std::wstring_view pattern) const
    {
        return is_valid(ParseBytePattern(pattern));
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

        return ScanResult(m_address - m_base_address, m_base_address, m_image_size);
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

    bool ScanResult::write(const std::string_view& data) const
    {
        return is_valid() ? Memory::Write(reinterpret_cast<void*>(m_address), data) : false;
    }
    
    bool ScanResult::write(const std::wstring_view& data) const
    {
        return is_valid() ? Memory::Write(reinterpret_cast<void*>(m_address), data) : false;
    }

    bool ScanResult::write(const std::initializer_list<uint8_t>& data) const
    {
        return is_valid() ? Memory::Write(reinterpret_cast<void*>(m_address), data) : false;
    }
    
    bool ScanResult::write(const std::vector<uint8_t>& data) const
    {
        return is_valid() ? Memory::Write(reinterpret_cast<void*>(m_address), data) : false;
    }

    PVOID* ScanResult::hook(PVOID hook_function) const
    {
        return (is_valid() && Hooking::HookFunction(&(PVOID&)m_address, hook_function)) ? reinterpret_cast<PVOID*>(m_address) : NULL;
    }

    bool ScanResult::unhook() const
    {
        return is_valid() ? Hooking::UnhookFunction(&(PVOID&)m_address) : false;
    }

    std::vector<ScanResult> ScanResult::get_all_references(const std::vector<BytePattern>& parsed_pattern, bool calculate_relative_address, uintptr_t base_address, size_t image_size, bool only_first) const
    {
        if (base_address == 0) base_address = m_base_address;
        if (image_size == 0) image_size = m_image_size;
        
        if (!calculate_relative_address) {
            std::vector<BytePattern> new_parsed_pattern = parsed_pattern;
            const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&m_address);
            for (size_t i = 0; i < sizeof(uintptr_t); ++i) {
                BytePattern bp;
                uint8_t byte = ptr[i];
                bp.half_byte[0].data = byte >> 4;
                bp.half_byte[1].data = byte & 0xF;
                new_parsed_pattern.push_back(bp);
            }
            return ScanAll(base_address, image_size, new_parsed_pattern, only_first);
        }

#if 0
        std::vector<ScanResult> result;
        const auto pattern_size = parsed_pattern.size();
        for (const auto& address : ScanAll(base_address, image_size, parsed_pattern)) {
            const auto offset_ptr = reinterpret_cast<int32_t*>(address.data() + pattern_size);
            const auto relative_address = address + pattern_size + *offset_ptr + sizeof(int32_t);
            if (relative_address == m_address) {
                result.push_back(ScanResult(address, base_address, image_size));
                if (only_first) break;
            }
        }
        return result;
#else
        std::vector<ScanResult> result;
        const uint8_t* start = reinterpret_cast<const uint8_t*>(base_address);
        const uint8_t* end = start + image_size;
        const size_t pattern_size = parsed_pattern.size();
        const BytePattern& first_pattern = parsed_pattern[0];

        for (const uint8_t* it = start; it + pattern_size <= end; ++it) {
            if ((first_pattern.half_byte[0].wildcard || ((it[0] & 0xF0) == (first_pattern.half_byte[0].data << 4))) &&
                (first_pattern.half_byte[1].wildcard || ((it[0] & 0x0F) == first_pattern.half_byte[1].data))) {
                
                bool found = true;
                for (size_t i = 1; i < pattern_size; ++i) {
                    const BytePattern& pattern = parsed_pattern[i];
                    uint8_t byte = it[i];

                    if (!(pattern.half_byte[0].wildcard || ((byte & 0xF0) == (pattern.half_byte[0].data << 4))) ||
                        !(pattern.half_byte[1].wildcard || ((byte & 0x0F) == pattern.half_byte[1].data))) {
                        found = false;
                        break;
                    }
                }

                if (found) {
                    const auto offset_ptr = reinterpret_cast<int32_t*>(const_cast<uint8_t*>(it + pattern_size));
                    const auto relative_address = reinterpret_cast<uintptr_t>(it) + pattern_size + *offset_ptr + sizeof(int32_t);
                    if (relative_address == m_address) {
                        result.push_back(ScanResult(reinterpret_cast<uintptr_t>(it), base_address, image_size));
                        if (only_first) {
                            break;
                        }
                    }
                }
            }
        }
        return result;
#endif
    }

    std::vector<ScanResult> ScanResult::get_all_references(std::wstring_view pattern, bool calculate_relative_address, uintptr_t base_address, size_t image_size, bool only_first) const
    {
        return get_all_references(ParseBytePattern(pattern), calculate_relative_address, base_address, image_size, only_first);
    }

    ScanResult ScanResult::get_first_reference(const std::vector<BytePattern>& parsed_pattern, bool calculate_relative_address, uintptr_t base_address, size_t image_size) const
    {
        const auto references = get_all_references(parsed_pattern, calculate_relative_address, base_address, image_size, true);
        return references.empty() ? ScanResult(0, 0, 0) : references.front();
    }
    
    ScanResult ScanResult::get_first_reference(std::wstring_view pattern, bool calculate_relative_address, uintptr_t base_address, size_t image_size) const
    {
        return get_first_reference(ParseBytePattern(pattern), calculate_relative_address, base_address, image_size);
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