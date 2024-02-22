#include "pch.h"

#ifndef NDEBUG

DWORD WINAPI Debug(LPVOID lpParam)
{
    try {
        const auto cef_request_t_get_url = offsetof(cef_request_t, get_url);
        const auto cef_zip_reader_get_file_name = offsetof(cef_zip_reader_t, get_file_name);
        const auto cef_zip_reader_t_read_file = offsetof(cef_zip_reader_t, read_file);

        if (cef_request_t_get_url != cef_request_t_get_url_offset) {
            PrintError(L"The offset of cef_request_t::get_url has changed: {}", cef_request_t_get_url);
        }
        if (cef_zip_reader_get_file_name != cef_zip_reader_get_file_name_offset) {
            PrintError(L"The offset of cef_zip_reader_t::get_file_name has changed: {}", cef_zip_reader_get_file_name);
        }
        if (cef_zip_reader_t_read_file != cef_zip_reader_t_read_file_offset) {
            PrintError(L"The offset of cef_zip_reader_t::read_file has changed: {}", cef_zip_reader_t_read_file);
        }

        //Utils::PrintSymbols(L"chrome_elf.dll");

        Utils::MeasureExecutionTime([&]() {



        });

    }
    catch (const std::exception& e) {
        Print(e.what());
    }
    return 0;
}
#endif