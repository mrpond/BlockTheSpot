#ifndef MEMORY_SCANNER_H
#define MEMORY_SCANNER_H

#include <Windows.h>
#include <string>
#include <vector>

namespace MemoryScanner 
{
    struct ModuleInfo {
        std::wstring_view module_name;
        uintptr_t base_address;
        size_t module_size;
    };

    class ScanResult {
    public:
        ScanResult() : m_address(0), m_base_address(0), m_image_size(0) {}
        ScanResult(uintptr_t address, uintptr_t base, size_t size);
        operator uintptr_t() const;

        bool is_valid(const std::vector<uint8_t>& value = {}) const;
        uint8_t* data() const;
        ScanResult rva() const;
        ScanResult offset(std::ptrdiff_t offset_value) const;
        ScanResult scan_first(std::wstring_view value) const;

        bool write(const void* data, size_t size) const;
        bool write(std::string_view data) const;
        bool write(std::initializer_list<uint8_t> data) const;

        PVOID* hook(PVOID hook_function) const;
        bool unhook() const;

        std::vector<ScanResult> get_all_matching_codes(const std::vector<uint8_t>& pattern_byte, bool calculate_relative_offset = true, uintptr_t base_address = 0, size_t image_size = 0, bool only_first = false) const;
        ScanResult get_first_matching_code(const std::vector<uint8_t>& pattern_byte, bool calculate_relative_offset = true, uintptr_t base_address = 0, size_t image_size = 0) const;

        uintptr_t get_base_address() const;
        size_t get_image_size() const;

        void print_address() const;

    private:
        uintptr_t m_address;
        uintptr_t m_base_address;
        size_t m_image_size;
    };

    ModuleInfo GetModuleInfo(std::wstring_view module_name = {});
    ScanResult GetFunctionAddress(std::string_view module_name, std::string_view function_name);
    
    std::vector<ScanResult> ScanAll(uintptr_t base_address, size_t image_size, const std::vector<uint8_t>& pattern_byte, bool only_first = false);
    std::vector<ScanResult> ScanAll(uintptr_t base_address, size_t image_size, std::wstring_view signature);
    std::vector<ScanResult> ScanAll(std::wstring_view signature, std::wstring_view module_name = {});
    
    ScanResult ScanFirst(uintptr_t base_address, size_t image_size, const std::vector<uint8_t>& pattern_byte);
    ScanResult ScanFirst(uintptr_t base_address, size_t image_size, std::wstring_view signature);
    ScanResult ScanFirst(std::wstring_view signature, std::wstring_view module_name = {});
}

#endif // MEMORY_SCANNER_H
