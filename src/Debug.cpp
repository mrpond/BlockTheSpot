#include "pch.h"

#ifndef NDEBUG

DWORD WINAPI Debug(LPVOID lpParam)
{
    try {
        const auto cef_request_t_get_url = offsetof(cef_request_t, get_url);
        const auto cef_zip_reader_t_get_file_name = offsetof(cef_zip_reader_t, get_file_name);
        const auto cef_zip_reader_t_read_file = offsetof(cef_zip_reader_t, read_file);

        if (cef_request_t_get_url != SettingsManager::m_cef_request_t_get_url_offset) {
            PrintError(L"The offset of cef_request_t::get_url has changed: {}", cef_request_t_get_url);
        }
        if (cef_zip_reader_t_get_file_name != SettingsManager::m_cef_zip_reader_t_get_file_name_offset) {
            PrintError(L"The offset of cef_zip_reader_t::get_file_name has changed: {}", cef_zip_reader_t_get_file_name);
        }
        if (cef_zip_reader_t_read_file != SettingsManager::m_cef_zip_reader_t_read_file_offset) {
            PrintError(L"The offset of cef_zip_reader_t::read_file has changed: {}", cef_zip_reader_t_read_file);
        }

        // Utils::PrintSymbols(L"chrome_elf.dll");

        Utils::MeasureExecutionTime([&]() {

            //for (const auto& pattern : MemoryScanner::ParseBytePattern((L"4? ?D 1?"))) {
            //    Print(L"HalfByte 1: Data = {}, Wildcard = {}", static_cast<int>(pattern.half_byte[0].data), pattern.half_byte[0].wildcard);
            //    Print(L"HalfByte 2: Data = {}, Wildcard = {}\n", static_cast<int>(pattern.half_byte[1].data), pattern.half_byte[1].wildcard);
            //}

            //MemoryScanner::ScanFirst(L"app-developer").get_first_reference(L"48 8D 15").print_address();
            //MemoryScanner::ScanFirst(L"app-developer").get_first_reference(L"4? ?D 1?").print_address();
            
            //for (const auto& it : MemoryScanner::ScanFirst(L"app-developer").get_all_references(L"68", false))
            //{
            //    it.print_address();
            //}

            //Print(L"{}", MemoryScanner::ScanFirst(L"55 8B EC 56 57 8B F1 33 C0 8B 4D 08 8B FE AB 8D 51 01 AB AB").get_all_references(L"E8").size());

        });
    }
    catch (const std::exception& e) {
        PrintError(L"{}", e.what());
    }
    return 0;
}
#endif