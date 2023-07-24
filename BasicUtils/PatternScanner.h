#ifndef _PATTERNSCANNER_H
#define _PATTERNSCANNER_H

#include <iostream>
#include <Windows.h>
#include <vector>
#include <format>
#include <span>

enum class AssemblyCode {
    PUSH_VALUE = 0x68,
    JUMP_IF_EQUAL = 0x74,
    JUMP_IF_NOT_EQUAL = 0x75,
    JUMP_RELATIVE_SHORT = 0xEB,
    JUMP_RELATIVE_LONG = 0xE9,
    CALL_RELATIVE = 0xE8
};

enum class ScanType {
    Unknown,    // The scan type is unknown
    Exact,      // The value must be exactly equal to the target value
    GreaterThan,// The value must be greater than the target value
    LessThan,   // The value must be less than the target value
    Between,    // The value must be between two target values
    Contains    // The value must contain the target value (only for string types)
};

enum class ValueType {
    Byte,
    Int,
    Float,
    Double,
    String,
};

class Scan {
public:
    Scan() = default;
    explicit Scan(std::uintptr_t address, std::pair<std::size_t, std::size_t> module_info);
    operator std::uintptr_t() const;

    void print_address(std::wstring_view name = {}) const;

    bool is_found() const;
    std::uint8_t* data() const;
    Scan rva() const;
    Scan offset(std::size_t value) const;
    Scan disassemble() const;

    bool hook(PVOID p_detours) const;
    bool unhook() const;

    std::vector<Scan> get_all_matching_codes(AssemblyCode code, std::size_t base_address = 0, std::size_t image_size = 0) const;
    Scan get_first_matching_code(AssemblyCode code, std::size_t base_address = 0, std::size_t image_size = 0) const;

    template <typename T>
    T read() const {
        if constexpr (std::is_pointer_v<T>)
            return reinterpret_cast<T>(m_address);
        else if constexpr (std::is_same_v<T, std::wstring> || std::is_same_v<T, std::wstring_view>)
            return T(reinterpret_cast<const char*>(m_address));
        else
            return *reinterpret_cast<const T*>(m_address);
    }

    template <typename T>
    bool write(const T& value) const {
        if constexpr (std::is_pointer_v<T> || std::is_same_v<T, std::wstring> || std::is_same_v<T, std::wstring_view>)
            return ::WriteProcessMemory(::GetCurrentProcess(), reinterpret_cast<LPVOID>(m_address), value, (std::char_traits<wchar_t>::length(reinterpret_cast<const wchar_t*>(value)) + 1) * sizeof(wchar_t), nullptr) != 0;
        else
            return ::WriteProcessMemory(::GetCurrentProcess(), reinterpret_cast<LPVOID>(m_address), std::addressof(value), sizeof(value), nullptr) != 0;
    }

private:
    std::uintptr_t m_address;
    std::pair<std::size_t, std::size_t> m_module_info;
    //const void* m_value;
};

class PatternScanner {
protected:
    struct ScanTargets {
        std::span<const std::uint8_t> first;
        std::span<const std::uint8_t> second;
        //std::span<const std::uint16_t> first;
        //std::span<const std::uint16_t> second;
    };

    static bool ScanMatch(const void* value, ScanTargets targets, ValueType value_type, ScanType scan_type);
public:
    struct ModuleInfo {
        std::size_t base_address;
        std::size_t image_size;
    };
    
    static ModuleInfo GetModuleInfo(std::wstring_view module_name = {});
    static Scan GetFunctionAddress(std::wstring_view module_name, std::wstring_view function_name);

    static std::vector<std::uint8_t> SignatureToByteArray(std::wstring_view signature);
    //static std::vector<std::uint16_t> SignatureToByteArray(std::wstring_view signature);
    static std::vector<Scan> ScanAll(std::size_t base_address, std::size_t image_size, ScanTargets byte_pattern, ValueType value_type, ScanType scan_type, bool forward);
    static std::vector<Scan> ScanAll(std::size_t base_address, std::size_t image_size, std::wstring_view value, ScanType scan_type = ScanType::Unknown, bool forward = true);
    static std::vector<Scan> ScanAll(std::wstring_view value, std::wstring_view module_name = {}, ScanType scan_type = ScanType::Unknown, bool forward = true);

    static Scan ScanFirst(std::size_t base_address, std::size_t image_size, ScanTargets byte_pattern, ValueType value_type, ScanType scan_type, bool forward);
    static Scan ScanFirst(std::size_t base_address, std::size_t image_size, std::wstring_view value, ScanType scan_type = ScanType::Unknown, bool forward = true);
    static Scan ScanFirst(std::wstring_view value, std::wstring_view module_name = {}, ScanType scan_type = ScanType::Unknown, bool forward = true);
};

namespace std {
    template<>
    struct formatter<Scan> : formatter<std::uintptr_t> {
        auto format(const Scan& scan, format_context& ctx) {
            return formatter<std::uintptr_t>::format(scan, ctx);
        }
    };
}

#endif // _PATTERNSCANNER_H