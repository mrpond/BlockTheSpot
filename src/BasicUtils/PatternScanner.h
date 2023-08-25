#ifndef _PATTERNSCANNER_H
#define _PATTERNSCANNER_H

#include <Windows.h>
#pragma warning(disable: 4530)
#include <iostream>
#include <vector>
#pragma warning(default: 4530)
#include <format>
#include <span>
#include "Memory.h"

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
    WString,
};

class Scan {
public:
    Scan() : m_address(0), m_module_info(0, 0) {}
    explicit Scan(std::uintptr_t address, std::pair<std::size_t, std::size_t> module_info);
    operator std::uintptr_t() const;

    void print_address(std::wstring_view name = {}) const;

    bool is_found(const std::vector<std::uint8_t>& value = {}) const;
    std::uint8_t* data() const;
    Scan rva() const;
    Scan offset(std::size_t value) const;
    Scan disassemble() const;

    void** hook(void* hook_function) const;
    bool unhook() const;

    Scan scan_first(std::wstring_view value, ScanType scan_type = ScanType::Unknown, bool forward = true) const;

    std::vector<Scan> get_all_matching_codes(std::vector<std::uint8_t> pattern, bool check_displacement = true, std::size_t base_address = 0, std::size_t image_size = 0) const;
    Scan get_first_matching_code(std::vector<std::uint8_t> pattern, bool check_displacement = true, std::size_t base_address = 0, std::size_t image_size = 0) const;

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
    bool write(const T& buffer, std::size_t bufferSize = -1) const {
        return is_found() ? Memory::Write(reinterpret_cast<LPVOID>(m_address), buffer, bufferSize) : false;
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