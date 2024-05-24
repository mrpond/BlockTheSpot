#include "Utils.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cwctype>
#include <fstream>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

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
        return ToHexWideString(std::vector<uint8_t>(data, data + size), insert_spaces);
    }

    std::vector<uint8_t> ToHexBytes(const std::string& hex_string)
    {
        std::vector<uint8_t> byte_array;

        std::istringstream iss(hex_string);
        std::string hex_byte;

        while (iss >> std::setw(2) >> hex_byte) {
            uint8_t byte_value = static_cast<uint8_t>(std::stoi(hex_byte, nullptr, 16));
            byte_array.push_back(byte_value);
        }

        return byte_array;
    }

    std::vector<uint8_t> ToHexBytes(const std::wstring& hex_wstring)
    {
        std::vector<uint8_t> byte_array;

        std::wistringstream iss(hex_wstring);
        std::wstring hex_byte;

        while (iss >> std::setw(2) >> hex_byte) {
            uint8_t byte_value = static_cast<uint8_t>(std::stoi(hex_byte, nullptr, 16));
            byte_array.push_back(byte_value);
        }

        return byte_array;
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

    bool ReadFile(const std::wstring_view filename, std::wstring& out)
    {
        std::wifstream file(filename.data(), std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;

        out.resize(static_cast<std::wstring::size_type>(file.tellg()));
        file.seekg(0, std::ios::beg);
        file.read(&out[0], out.size());

        return !file.fail();
    }

    bool WriteFile(const std::wstring_view filename, const std::wstring_view content)
    {
        std::wofstream file(filename.data(), std::ios::binary);
        if (!file.is_open()) return false;

        file.write(content.data(), content.size());

        return !file.fail();
    }

    std::wstring HttpGetRequest(std::wstring_view url)
    {
        std::wstring response;

        auto session = WinHttpOpen(L"WinHTTP", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!session) {
            PrintError(L"Failed to initialize WinHTTP session");
            return response;
        }

        URL_COMPONENTS url_comp;
        ZeroMemory(&url_comp, sizeof(url_comp));
        url_comp.dwStructSize = sizeof(url_comp);
        url_comp.dwHostNameLength = -1;
        url_comp.dwUrlPathLength = -1;

        if (!WinHttpCrackUrl(url.data(), static_cast<DWORD>(url.length()), 0, &url_comp)) {
            PrintError(L"Failed to parse URL");
            WinHttpCloseHandle(session);
            return response;
        }

        std::wstring host(url_comp.lpszHostName, url_comp.dwHostNameLength);
        std::wstring path(url_comp.lpszUrlPath, url_comp.dwUrlPathLength);

        auto connect = WinHttpConnect(session, host.c_str(), url_comp.nPort, 0);
        if (!connect) {
            PrintError(L"Failed to connect to host");
            WinHttpCloseHandle(session);
            return response;
        }

        auto request = WinHttpOpenRequest(connect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            url_comp.nPort == INTERNET_DEFAULT_HTTPS_PORT ? WINHTTP_FLAG_SECURE : 0);
        if (!request) {
            PrintError(L"Failed to open request");
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return response;
        }

        if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            PrintError(L"Failed to send request");
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return response;
        }

        if (!WinHttpReceiveResponse(request, NULL)) {
            PrintError(L"Failed to receive response");
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return response;
        }

        DWORD size = 0;
        DWORD downloaded = 0;
        std::vector<char> buffer(1024);

        do {
            if (!WinHttpQueryDataAvailable(request, &size)) {
                PrintError(L"Failed to query data availability");
                WinHttpCloseHandle(request);
                WinHttpCloseHandle(connect);
                WinHttpCloseHandle(session);
                return response;
            }

            if (size == 0) {
                break;
            }

            if (size > buffer.size()) {
                buffer.resize(size);
            }

            if (WinHttpReadData(request, buffer.data(), size, &downloaded)) {
                response.append(buffer.begin(), buffer.begin() + downloaded);
            } else {
                PrintError(L"Failed to read data");
                WinHttpCloseHandle(request);
                WinHttpCloseHandle(connect);
                WinHttpCloseHandle(session);
                return response;
            }
        } while (size > 0);

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);

        return response;
    }

#ifndef NDEBUG
    void MeasureExecutionTime(std::function<void()> func, bool total_duration)
    {
        const auto start_time = std::chrono::high_resolution_clock::now();
        func();
        const auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end_time - start_time;
        if (total_duration) {
            static std::chrono::duration<double> total_diff;
            total_diff += diff;
            SetConsoleTitleW(FormatString(L"Total execution time: {:f} seconds", total_diff.count()).c_str());
        }
        else {
            SetConsoleTitleW(FormatString(L"Execution time: {:f} seconds", diff.count()).c_str());
        }
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