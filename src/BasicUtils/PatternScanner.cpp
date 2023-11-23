#include "PatternScanner.h"
#include "Memory.h"
#include "Console.h"
#include "Hooking.h"
#include "Utils.h"
#include <Psapi.h>
#pragma warning(disable: 4530)
#include <sstream>
#pragma warning(default: 4530)
#include <iomanip>
#include <regex>

ModuleInfo PatternScanner::GetModuleInfo(std::wstring_view module_name)
{
    for (const auto& it : m_module_info) {
        if (it.module_name == module_name) {
            return ModuleInfo(it.module_name, it.base_address, it.module_size);
        }
    }

    HMODULE module_handle = GetModuleHandleW(module_name.empty() ? nullptr : module_name.data());
    if (module_handle == nullptr) {
        return ModuleInfo(module_name, 0, 0);
    }

    MODULEINFO module_info;
    if (!GetModuleInformation(GetCurrentProcess(), module_handle, &module_info, sizeof(MODULEINFO))) {
        return ModuleInfo(module_name, 0, 0);
    }

    const auto ret = ModuleInfo(module_name, reinterpret_cast<uintptr_t>(module_info.lpBaseOfDll), static_cast<size_t>(module_info.SizeOfImage));
    m_module_info.push_back(ret);
    return ret;
}

Scan PatternScanner::GetFunctionAddress(std::wstring_view module_name, std::wstring_view function_name)
{
    HMODULE module_handle = GetModuleHandleW(module_name.data());
    if (!module_handle) {
        module_handle = LoadLibraryW(module_name.data());
        if (!module_handle) {
            PrintError(L"GetFunctionAddress: Failed to load module");
            return Scan();
        }
    }

    FARPROC function_address = GetProcAddress(module_handle, Utils::ToString(function_name).c_str());
    if (!function_address) {
        PrintError(L"GetFunctionAddress: Failed to get function address");
        return Scan();
    }

    MODULEINFO module_info;
    if (!GetModuleInformation(GetCurrentProcess(), module_handle, &module_info, sizeof(module_info))) {
        PrintError(L"GetFunctionAddress: Failed to get module information");
        return Scan();
    }

    return Scan(reinterpret_cast<std::uintptr_t>(function_address), { reinterpret_cast<uintptr_t>(module_info.lpBaseOfDll), static_cast<size_t>(module_info.SizeOfImage) });
}

std::vector<Scan> PatternScanner::ScanAll(uintptr_t base_address, size_t image_size, std::wstring_view signature)
{
    std::vector<Scan> matches;

    std::vector<uint8_t> pattern_byte = SignatureToByteArray(signature);
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
            matches.push_back(Scan(reinterpret_cast<uintptr_t>(base_ptr), std::make_pair(base_address, image_size)));
        }

        ++base_ptr;
    }

    return matches;
}

std::vector<Scan> PatternScanner::ScanAll(std::wstring_view signature, std::wstring_view module_name)
{
    auto mod = GetModuleInfo(module_name);
    return ScanAll(mod.base_address, mod.module_size, signature);
}

Scan PatternScanner::ScanFirst(uintptr_t base_address, size_t image_size, std::wstring_view signature)
{
    std::vector<uint8_t> pattern_byte = SignatureToByteArray(signature);
    size_t pattern_size = pattern_byte.size();

    if (pattern_byte.empty()) {
        return Scan(0, std::make_pair(0, 0));
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
            return Scan(reinterpret_cast<uintptr_t>(base_ptr), std::make_pair(base_address, image_size));
        }

        ++base_ptr;
    }

    return Scan(0, std::make_pair(0, 0));
}

Scan PatternScanner::ScanFirst(std::wstring_view signature, std::wstring_view module_name)
{
    auto mod = GetModuleInfo(module_name);
    return ScanFirst(mod.base_address, mod.module_size, signature);
}

std::vector<uint8_t> PatternScanner::SignatureToByteArray(std::wstring_view signature)
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

Scan::Scan(std::uintptr_t address, std::pair<std::size_t, std::size_t> module_info) : m_address(address), m_module_info(module_info)/*, m_value(reinterpret_cast<const void*>(address))*/
{
    //...
}

Scan::operator std::uintptr_t() const
{
    return m_address;
}

void Scan::print_address(std::wstring_view name) const
{
    Print({ Color::Yellow , Color::Cyan }, L"{:X} {}", m_address, !name.empty() ? L" : " + std::wstring(name.data()) : L"");
}

bool Scan::is_found(const std::vector<std::uint8_t>& value) const
{
    if (m_address == NULL)
        return false;

    if (!value.empty()) {
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (*(reinterpret_cast<std::uint8_t*>(m_address) + i) != value[i])
                return false;
        }
    }

    return true;
}

std::uint8_t* Scan::data() const
{
    return reinterpret_cast<std::uint8_t*>(m_address);
}

Scan Scan::rva() const
{
    return is_found() ? Scan(m_address - m_module_info.first, m_module_info) : Scan(NULL, m_module_info);
}

Scan Scan::offset(std::size_t value) const
{
    if (is_found()) {
        if (value > 0) {
            return Scan(m_address + value, m_module_info);
        }
        else {
            return Scan(m_address - value, m_module_info);
        }
    }
    else {
        return Scan(NULL, m_module_info);
    }
}

void** Scan::hook(void* hook_function) const
{
    return (is_found() && Hooking::HookFunction(&(void*&)m_address, hook_function)) ? reinterpret_cast<void**>(m_address) : NULL;
}

bool Scan::unhook() const
{
    return is_found() ? Hooking::UnhookFunction(&(PVOID&)m_address) : false;
}

Scan Scan::scan_first(std::wstring_view value) const
{
    return is_found() ? PatternScanner::ScanFirst(m_address, m_module_info.second - rva(), value) : Scan(NULL, m_module_info);
}

std::vector<Scan> Scan::get_all_matching_codes(std::vector<std::uint8_t> pattern, bool check_displacement, std::size_t base_address, std::size_t image_size) const
{
    std::vector<Scan> addresses;
    auto pattern_scan = PatternScanner::ScanAll(base_address == 0 ? m_module_info.first : base_address, image_size == 0 ? m_module_info.second : image_size, Utils::ToHexWideString(pattern));
    for (const auto& it : pattern_scan) {
        const auto instruction_address = static_cast<std::int32_t>(it);
        const auto target_address = static_cast<std::int32_t>(m_address);
        const auto displacement = target_address - instruction_address - pattern.size() - sizeof(std::int32_t);
        const auto expected_value = check_displacement ? displacement : target_address;
        if (*reinterpret_cast<const std::int32_t*>(it + pattern.size()) == expected_value)
            addresses.push_back(Scan(it, { base_address == 0 ? m_module_info.first : base_address, image_size == 0 ? m_module_info.second : image_size }));
    }
    return addresses;
}

Scan Scan::get_first_matching_code(std::vector<std::uint8_t> pattern, bool check_displacement, std::size_t base_address, std::size_t image_size) const
{
    auto pattern_scan = PatternScanner::ScanAll(base_address == 0 ? m_module_info.first : base_address, image_size == 0 ? m_module_info.second : image_size, Utils::ToHexWideString(pattern));
    for (const auto& it : pattern_scan) {
        const auto instruction_address = static_cast<std::int32_t>(it);
        const auto target_address = static_cast<std::int32_t>(m_address);
        const auto displacement = target_address - instruction_address - pattern.size() - sizeof(std::int32_t);
        const auto expected_value = check_displacement ? displacement : target_address;
        if (*reinterpret_cast<const std::int32_t*>(it + pattern.size()) == expected_value)
            return Scan(it, { base_address == 0 ? m_module_info.first : base_address, image_size == 0 ? m_module_info.second : image_size });
    }
    return Scan();
}

std::vector<ModuleInfo> PatternScanner::m_module_info = {};