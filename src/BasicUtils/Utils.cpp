#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

#include "Utils.h"
#include "Console.h"
#include <Windows.h>
#include <chrono>
#include <codecvt>
#include <fstream>

using namespace Console;

namespace Utils
{
    std::wstring ToHexWideString(const std::vector<uint8_t>& byte_array, const bool insert_spaces)
    {
        std::wostringstream oss;
        oss << std::hex << std::setfill(L'0');

        for (size_t i = 0; i < byte_array.size(); ++i)
        {
            if (i > 0 && insert_spaces)
            {
                oss << L' ';
            }
            oss << std::setw(2) << static_cast<int>(byte_array[i]);
        }

        std::wstring hex_string = oss.str();
        std::transform(hex_string.begin(), hex_string.end(), hex_string.begin(), ::towupper);

        return hex_string;
    }
    
    std::string ToHexString(const std::vector<uint8_t>& byte_array, const bool insert_spaces)
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        for (size_t i = 0; i < byte_array.size(); ++i)
        {
            if (i > 0 && insert_spaces)
            {
                oss << ' ';
            }
            oss << std::setw(2) << static_cast<int>(byte_array[i]);
        }

        std::string hex_string = oss.str();
        std::transform(hex_string.begin(), hex_string.end(), hex_string.begin(), ::toupper);

        return hex_string;
    }

    std::string ToHexString(const uint8_t* data, size_t size, const bool insert_spaces)
    {
        if (data == nullptr)
            throw std::invalid_argument("The data pointer is null.");

        if (size == 0)
            size = std::strlen(reinterpret_cast<const char*>(data));

        return ToHexString(std::vector<uint8_t>(data, data + size), insert_spaces);
    }

    std::wstring ToHexWideString(const uint8_t* data, size_t size, const bool insert_spaces)
    {
        if (data == nullptr)
            throw std::invalid_argument("The data pointer is null.");

        if (size == 0)
            size = std::wcslen(reinterpret_cast<const wchar_t*>(data));

        return ToHexWideString(std::vector<uint8_t>(data, data + size), insert_spaces);
    }

    std::string ConvertUInt8ArrayToString(std::uint8_t* data)
    {
        return std::string(reinterpret_cast<char*>(data), reinterpret_cast<char*>(data + std::strlen(reinterpret_cast<char*>(data))));
    }

    std::wstring ConvertUInt8ArrayToWideString(std::uint8_t* data)
    {
        return std::wstring(reinterpret_cast<wchar_t*>(data), reinterpret_cast<wchar_t*>(data + std::wcslen(reinterpret_cast<wchar_t*>(data))));
    }

    std::string ToString(std::wstring_view wide_string)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(wide_string.data(), wide_string.data() + wide_string.size());
    }

    std::wstring ToString(std::string_view narrow_string)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(narrow_string.data(), narrow_string.data() + narrow_string.size());
    }

    std::wstring ToString(std::u16string_view utf16_string)
    {
        std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>> converter;
        return converter.from_bytes(reinterpret_cast<const char*>(utf16_string.data()), reinterpret_cast<const char*>(utf16_string.data() + utf16_string.size()));
    }

    bool Contains(std::string_view str1, std::string_view str2, bool case_sensitive)
    {
        auto it = std::search(
            str1.begin(), str1.end(),
            str2.begin(), str2.end(),
            [case_sensitive](char ch1, char ch2) {
                return case_sensitive ? ch1 == ch2 : std::toupper(ch1) == std::toupper(ch2);
            }
        );
        return (it != str1.end());
    }

    bool Contains(std::wstring_view str1, std::wstring_view str2, bool case_sensitive)
    {
        auto it = std::search(
            str1.begin(), str1.end(),
            str2.begin(), str2.end(),
            [case_sensitive](wchar_t ch1, wchar_t ch2) {
                return case_sensitive ? ch1 == ch2 : std::toupper(ch1) == std::toupper(ch2);
            }
        );
        return (it != str1.end());
    }

    bool Equals(std::string_view str1, std::string_view str2, bool case_sensitive)
    {
        return std::equal(str1.begin(), str1.end(), str2.begin(), str2.end(),
            [case_sensitive](char ch1, char ch2) {
                return case_sensitive ? ch1 == ch2 : std::toupper(ch1) == std::toupper(ch2);
            });
    }

    bool Equals(std::wstring_view str1, std::wstring_view str2, bool case_sensitive)
    {
        return std::equal(str1.begin(), str1.end(), str2.begin(), str2.end(),
            [case_sensitive](wchar_t ch1, wchar_t ch2) {
                return case_sensitive ? ch1 == ch2 : std::toupper(ch1) == std::toupper(ch2);
            });
    }

    void MeasureExecutionTime(std::function<void()> func, std::wstring_view name)
    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        func();
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
        Print({ Color::White, Color::White, Color::Yellow, Color::White }, L"{}{}{}{}Execution time: {:d}ms\n", info.dwCursorPosition.Y != 0 ? L"\n" : L"",
            !name.empty() ? L"[" : L"",
            !name.empty() ? FormatString(L"{}", name).c_str() : L"",
            !name.empty() ? L"] " : L"", duration);
    }

    IniData ReadIniFile(const std::wstring& fileName)
    {
        IniData data;
        std::wstring currentSection;

        std::wifstream file(fileName);

        if (!file) {
            throw std::runtime_error("Failed to open ini file.");
        }

        for (std::wstring line; getline(file, line);) {
            if (line.empty() || line[0] == ';') {
                continue;
            }

            if (line[0] == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.size() - 2);
                data[currentSection] = {};
            }
            else {
                size_t pos = line.find('=');
                if (pos == std::wstring::npos) {
                    continue;
                }

                std::wstring key = line.substr(0, pos);
                std::wstring valueStr = line.substr(pos + 1);

                if (key.empty() || valueStr.empty()) {
                    continue;
                }

                bool value = (valueStr == L"true" || valueStr == L"1");

                data[currentSection][key] = value;
            }
        }

        return data;
    }

    void AppendIniFile(const std::wstring& fileName, IniData& data)
    {
        IniData existingData;
    
        if (std::wifstream existingFile(fileName); existingFile) {
            existingData = ReadIniFile(fileName);
        }

        for (auto& [sectionName, sectionData] : existingData) {
            for (auto it = sectionData.begin(); it != sectionData.end();) {
                const auto& [key, value] = *it;
                if (data[sectionName].find(key) == data[sectionName].end()) {
                    it = sectionData.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    
        std::wofstream file(fileName);
    
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open ini file.");
        }
    
        for (const auto& [sectionName, sectionData] : existingData) {
            file << L"[" << sectionName << L"]" << '\n';
    
            for (const auto& [key, value] : sectionData) {
                file << key << L"=" << (value ? L"1" : L"0") << '\n';
                data[sectionName][key] = value;
            }
        }
    
        for (const auto& [sectionName, sectionData] : data) {
            if (existingData.find(sectionName) == existingData.end()) {
                file << L"[" << sectionName << L"]" << '\n';
            }
    
            for (const auto& [key, value] : sectionData) {
                if (existingData[sectionName].find(key) == existingData[sectionName].end()) {
                    file << key << L"=" << (value ? L"1" : L"0") << '\n';
                }
            }
        }
    }

#ifndef NDEBUG
    void PrintSymbols(std::wstring_view module_name)
    {
        HMODULE hModule = GetModuleHandleW(module_name.data());
        if (!hModule && !(hModule = LoadLibraryW(module_name.data())))
            throw std::runtime_error("Failed to load module.");

        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
        PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)dosHeader + dosHeader->e_lfanew);
        PIMAGE_EXPORT_DIRECTORY exportDirectory = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)dosHeader + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
        PDWORD functions = (PDWORD)((BYTE*)dosHeader + exportDirectory->AddressOfFunctions);
        PDWORD names = (PDWORD)((BYTE*)dosHeader + exportDirectory->AddressOfNames);
        PWORD ordinals = (PWORD)((BYTE*)dosHeader + exportDirectory->AddressOfNameOrdinals);

        for (DWORD i = 0; i < exportDirectory->NumberOfNames; i++) {
            Print(L"{}", reinterpret_cast<const char*>((BYTE*)dosHeader + names[i]));
        }
    }
#endif

    void SetLocaleToUTF8()
    {
#if defined(_DEBUG) || defined(_CONSOLE)
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
#endif
        std::locale::global(std::locale("en_US.UTF-8"));
    }
}