#ifndef _UTILS_H
#define _UTILS_H

#include <Windows.h>
#include <iostream>
#include <string>
#include <format>
#include <functional>

namespace Utils
{
    std::string ToHexString(const std::vector<uint8_t>& byte_array, const bool insert_spaces);
    std::string ToHexString(const uint8_t* data, size_t size, const bool insert_spaces);
    
    std::wstring ToHexWideString(const std::vector<uint8_t>& byte_array, const bool insert_spaces);
    std::wstring ToHexWideString(const uint8_t* data, size_t size, const bool insert_spaces);

    std::string ConvertUInt8ArrayToString(uint8_t* data);
    std::wstring ConvertUInt8ArrayToWideString(uint8_t* data);

    std::string IntegerToHexString(uintptr_t integer_value);
    std::wstring IntegerToHexWideString(uintptr_t integer_value);

    std::string ToString(std::wstring_view wide_string);
    std::wstring ToString(std::string_view narrow_string);
    std::wstring ToString(std::u16string_view utf16_string);

    bool Contains(std::string_view str1, std::string_view str2, bool case_sensitive);
    bool Contains(std::wstring_view str1, std::wstring_view str2, bool case_sensitive);

    bool Equals(std::string_view str1, std::string_view str2, bool case_sensitive);
    bool Equals(std::wstring_view str1, std::wstring_view str2, bool case_sensitive);

	void WriteIniFile(std::wstring_view ini_path, std::wstring_view section, std::wstring_view key, std::wstring_view value);
	std::wstring ReadIniFile(std::wstring_view ini_path, std::wstring_view section, std::wstring_view key);

#ifndef NDEBUG
    //Example: MeasureExecutionTime([&]() { Function(arg1, arg2); });
    void MeasureExecutionTime(std::function<void()> func);
    
    void PrintSymbols(std::wstring_view module_name);
#endif

    template<typename T>
    constexpr auto TypeConvert(const T& arg)
    {
        if constexpr (std::is_same_v<T, const wchar_t*>) {
            return std::wstring_view(arg);
        }
        else if constexpr (std::is_same_v<T, const char*>) {
            return ToString(arg);
        }
        else if constexpr (std::is_same_v < T, void*>) {
            return reinterpret_cast<uintptr_t>(arg);
        }
        else if constexpr (std::is_pointer_v<T>) {
            return *arg;
        }
        else {
            return arg;
        }
    }

    std::string FormatString(std::string_view fmt, const auto&... args)
    {
        return std::vformat(fmt, std::make_format_args(args...));
    }

    std::wstring FormatString(std::wstring_view fmt, const auto&... args)
    {
        return std::vformat(fmt, std::make_wformat_args(TypeConvert(args)...));
    }

    enum class Color : WORD
    {
        Red = FOREGROUND_RED,
        Green = FOREGROUND_GREEN,
        Blue = FOREGROUND_BLUE,
        Yellow = FOREGROUND_RED | FOREGROUND_GREEN,
        Cyan = FOREGROUND_GREEN | FOREGROUND_BLUE,
        Magenta = FOREGROUND_RED | FOREGROUND_BLUE,
        White = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        Black = 0,
        Gray = FOREGROUND_INTENSITY,
        DarkRed = FOREGROUND_RED | FOREGROUND_INTENSITY,
        DarkGreen = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        DarkBlue = FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        DarkYellow = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        DarkCyan = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        DarkMagenta = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
    };

#if defined(_DEBUG) || defined(_CONSOLE)
    void _Print(std::string_view fmt, const auto&... args)
    {
        std::cout << FormatString(fmt, args...) << std::endl;
    }

    void _Print(std::wstring_view fmt, const auto&... args)
    {
        std::wcout << FormatString(fmt, args...) << std::endl;
    }

    void _Print(const std::vector<Color>& colors, std::wstring fmt, const auto&... args)
    {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to get console handle");
        }

        size_t start = 0;
        size_t pos = fmt.find(L"{");
        size_t color_index = 0;

        auto print_arg = [&](const auto& arg) {
            if (pos != std::wstring::npos) {
                std::wcout << fmt.substr(start, pos - start);
                if (color_index < colors.size()) {
                    if (!SetConsoleTextAttribute(hConsole, static_cast<WORD>(colors[color_index]))) {
                        throw std::runtime_error("Failed to set console text attribute");
                    }
                    ++color_index;
                }

                size_t end = fmt.find(L"}", pos);
                if (end != std::wstring::npos) {
                    std::wstring formatSpecifier = fmt.substr(pos, end - pos + 1);
                    std::wcout << Utils::FormatString(formatSpecifier, arg);
                    pos = fmt.find(L"{", end + 1);
                }
                else {
                    throw std::runtime_error("Invalid format string");
                }

                if (!SetConsoleTextAttribute(hConsole, static_cast<WORD>(Color::White))) {
                    throw std::runtime_error("Failed to set console text attribute");
                }
                start = end + 1;
            }
            else {
                std::wcout << fmt.substr(start);
            }
            };

        (print_arg(args), ...);
        std::wcout << fmt.substr(start) << std::endl;
    }
#endif
};

using Utils::Color;

#if defined(_DEBUG) || defined(_CONSOLE)
#define Print(fmt, ...) Utils::_Print(fmt, __VA_ARGS__)
#define PrintColor(colors, fmt, ...) Utils::_Print(colors, fmt, __VA_ARGS__)
#define PrintError(fmt, ...) Utils::_Print({ Color::Red }, L"{} " fmt, L" -", __VA_ARGS__)
#define PrintStatus(flag, label) Utils::_Print({ (flag) ? Color::Green : Color::Red }, L"{} " label, (flag) ? L" +" : L" -")
#else
#define Print(fmt, ...)
#define PrintColor(colors, fmt, ...)
#define PrintError(fmt, ...)
#define PrintStatus(flag, label)
#endif

#endif // _UTILS_H