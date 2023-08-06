#include "pch.h"

#ifndef NDEBUG

#include <intrin.h>

//void print_test() {
//    // Example colors vector
//    std::vector<Color> colors = { Color::Red, Color::Green, Color::Blue };
//
//    // Example 6: Uppercase hexadecimal formatting
//    int hex_value = 255;
//    Print(colors, L"The uppercase hexadecimal value is {:X}.", hex_value);
//    // Output: The uppercase hexadecimal value is FF.
//
//    // Example 7: Hexadecimal formatting with "0x" prefix
//    int hex_value_with_prefix = 123;
//    Print(colors, L"The hexadecimal value with prefix is {:#x}.", hex_value_with_prefix);
//    // Output: The hexadecimal value with prefix is 0x7b.
//
//    // Example 8: Multiple arguments with different format specifiers
//    double temperature = 23.5;
//    int year = 2023;
//    std::wstring location = L"New York";
//    Print(colors, L"Date: {}, Temperature: {:.1f} °C, Year: {:d}, Location: {}.", L"July 22", temperature, year, location);
//    // Output: Date: July 22, Temperature: 23.5 °C, Year: 2023, Location: New York.
//
//    // Example 9: Handling missing arguments
//    Print(colors, L"{} + {} = {}.", 2, 3);
//    // Output: 2 + 3 = {}.
//
//    // Example 10: Using the same argument multiple times
//    int value = 7;
//    Print(colors, L"{} * {} = {} and {} + {} = {}.", value, 3, value * 3, value, 5, value + 5);
//    // Output: 7 * 3 = 21 and 7 + 5 = 12.
//
//    // Example 11: Using non-default colors
//    std::vector<Color> custom_colors = { Color::Yellow, Color::Magenta };
//    Print(custom_colors, L"Custom colors: {} and {}.", L"Yellow", L"Magenta");
//    // Output: Custom colors: Yellow and Magenta.
//}

//typedef void(__stdcall* cef_string_utf16_clear_t)(void*);
//cef_string_utf16_clear_t cef_string_utf16_clear_orig = nullptr;
//
//void __stdcall hook_cef_string_utf16_clear(void* str) {
//    std::wstring wstr(*reinterpret_cast<wchar_t**>(str));
//    if (wstr == L"xpui.js") {
//        Print({ Color::Yellow }, L"[{}]: {:#x} | {}", L"cef_string_utf16_clear", _ReturnAddress(), wstr);
//        _wsystem(L"pause");
//    }
//
//    return cef_string_utf16_clear_orig(str);
//}

DWORD WINAPI Debug(LPVOID lpParam)
{
    try
    {
        Utils::MeasureExecutionTime([&]() {
            //Utils::PrintSymbols(L"chrome_elf.dll");
            //print_test();

            //const auto cef_string_utf16_clear_func = PatternScanner::GetFunctionAddress(L"libcef.dll", L"cef_string_utf16_clear");
            //cef_string_utf16_clear_orig = (cef_string_utf16_clear_t)cef_string_utf16_clear_func.data();
            //cef_string_utf16_clear_func.hook((PVOID)hook_cef_string_utf16_clear);

            }, L"DEBUG");
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
#endif