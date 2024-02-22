#include "Utils.h"

#include <algorithm>
#include <chrono>

namespace Utils
{
    std::string ToHexString(const std::vector<uint8_t>& byte_array, const bool insert_spaces)
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        for (size_t i = 0; i < byte_array.size(); ++i) {
            if (i > 0 && insert_spaces) {
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
        if (data == nullptr) {
            PrintError(L"ToHexString: The data pointer is null.");
            return std::string();
        }

        if (size == 0)
            size = std::strlen(reinterpret_cast<const char*>(data));

        return ToHexString(std::vector<uint8_t>(data, data + size), insert_spaces);
    }

    std::wstring ToHexWideString(const std::vector<uint8_t>& byte_array, const bool insert_spaces)
    {
        std::wostringstream oss;
        oss << std::hex << std::setfill(L'0');

        for (size_t i = 0; i < byte_array.size(); ++i) {
            if (i > 0 && insert_spaces) {
                oss << L' ';
            }
            oss << std::setw(2) << static_cast<int>(byte_array[i]);
        }

        std::wstring hex_string = oss.str();
        std::transform(hex_string.begin(), hex_string.end(), hex_string.begin(), ::towupper);

        return hex_string;
    }

    std::wstring ToHexWideString(const uint8_t* data, size_t size, const bool insert_spaces)
    {
        if (data == nullptr) {
            PrintError(L"ToHexWideString: The data pointer is null.");
            return std::wstring();
        }

        if (size == 0)
            size = std::wcslen(reinterpret_cast<const wchar_t*>(data));

        return ToHexWideString(std::vector<uint8_t>(data, data + size), insert_spaces);
    }

    std::string ConvertUInt8ArrayToString(uint8_t* data)
    {
        return std::string(reinterpret_cast<char*>(data), reinterpret_cast<char*>(data + std::strlen(reinterpret_cast<char*>(data))));
    }

    std::wstring ConvertUInt8ArrayToWideString(uint8_t* data)
    {
        return std::wstring(reinterpret_cast<wchar_t*>(data), reinterpret_cast<wchar_t*>(data + std::wcslen(reinterpret_cast<wchar_t*>(data))));
    }

    std::string IntegerToHexString(uintptr_t integer_value)
    {
        return std::format("{:x}", integer_value);
    }

    std::wstring IntegerToHexWideString(uintptr_t integer_value)
    {
        return std::format(L"{:x}", integer_value);
    }

    std::string ToString(std::wstring_view wide_string)
    {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], (int)wide_string.size(), NULL, 0, NULL, NULL);
        std::string str_to(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wide_string[0], (int)wide_string.size(), &str_to[0], size_needed, NULL, NULL);
        return str_to;
    }

    std::wstring ToString(std::string_view narrow_string)
    {
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &narrow_string[0], (int)narrow_string.size(), NULL, 0);
        std::wstring wstr_to(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &narrow_string[0], (int)narrow_string.size(), &wstr_to[0], size_needed);
        return wstr_to;
    }

    std::wstring ToString(std::u16string_view utf16_string)
    {
        return std::wstring(utf16_string.begin(), utf16_string.end());
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

	void WriteIniFile(std::wstring_view ini_path, std::wstring_view section, std::wstring_view key, std::wstring_view value)
	{
		WritePrivateProfileStringW(section.data(), key.data(), value.data(), ini_path.data());
	}

	std::wstring ReadIniFile(std::wstring_view ini_path, std::wstring_view section, std::wstring_view key)
	{
		wchar_t value[255];
		GetPrivateProfileStringW(section.data(), key.data(), L"", value, 255, ini_path.data());
		return std::wstring(value);
	}

#ifndef NDEBUG
    void MeasureExecutionTime(std::function<void()> func)
    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        func();
        const auto end_time = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        SetConsoleTitleW(FormatString(L"Execution time: {:d}ms", duration).c_str());
    }

    void PrintSymbols(std::wstring_view module_name)
    {
        HMODULE hModule = GetModuleHandleW(module_name.data());
        if (!hModule && !(hModule = LoadLibraryW(module_name.data()))) {
            PrintError(L"PrintSymbols: Failed to load module.");
            return;
        }
        
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
#endif // NDEBUG
};