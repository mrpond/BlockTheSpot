#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <Windows.h>
#include <iostream>
#include <string_view>
#include <vector>
#include <mutex>
//#include <regex>
#include <format>
#include "Utils.h"

namespace Console {
	extern std::mutex cout_mutex;

	enum class Color : WORD {
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
	// Converts a value to the appropriate type for std::vformat()
	template<typename T>
	constexpr auto TypeConvert(const T& arg) {
		if constexpr (std::is_same_v<T, const wchar_t*>) {
			return std::wstring_view(arg);
		}
		else if constexpr (std::is_same_v<T, char*> || std::is_same_v<T, const char*> || std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
			return Utils::ToString(arg);
		}
		else if constexpr (std::is_same_v < T, void*>) {
			return reinterpret_cast<std::uintptr_t>(arg);
		}
		else if constexpr (std::is_pointer_v<T>) {
			return *arg;
		}
		else {
			return arg;
		}
	}
#endif

	/**
	* The format string can contain various format specifiers such as:
	* - "{}": Default representation of the argument.
	* - "{:d}": Decimal integer.
	* - "{:x}/{:X}": Hexadecimal integer (lowercase/uppercase).
	* - "{:#x}/{:#X}": Hexadecimal integer with "0x"/"0X" prefix.
	* - "{:.2f}": Fixed-point floating-point with 2 digits after the decimal point.
	*/
	void Print(std::wstring_view fmt, const auto&... args)
	{
#if defined(_DEBUG) || defined(_CONSOLE)
		try {
			std::scoped_lock lock(cout_mutex);
			std::wcout << std::vformat(fmt, std::make_wformat_args(TypeConvert(args)...)) << std::endl;
		}
		catch (const std::exception& e) {
			throw std::runtime_error("Failed to format string: " + std::string(e.what()));
		}
#endif
	}

	/**
	* The format string can contain various format specifiers such as:
	* - "{}": Default representation of the argument.
	* - "{:d}": Decimal integer.
	* - "{:x}/{:X}": Hexadecimal integer (lowercase/uppercase).
	* - "{:#x}/{:#X}": Hexadecimal integer with "0x"/"0X" prefix.
	* - "{:.2f}": Fixed-point floating-point with 2 digits after the decimal point.
	*/
	void Print(const std::vector<Color>& colors, std::wstring fmt, const auto&... args) {
#if defined(_DEBUG) || defined(_CONSOLE)
		try {
			std::scoped_lock lock(cout_mutex);
			HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			if (hConsole == INVALID_HANDLE_VALUE) {
				throw std::runtime_error("Failed to get console handle");
			}

			size_t start = 0;
			size_t pos = fmt.find(L"{");
			size_t colorIndex = 0;

			auto printArg = [&](const auto& arg) {
				if (pos != std::wstring::npos) {
					std::wcout << fmt.substr(start, pos - start);
					if (colorIndex < colors.size()) {
						if (!SetConsoleTextAttribute(hConsole, static_cast<WORD>(colors[colorIndex]))) {
							throw std::runtime_error("Failed to set console text attribute");
						}
						++colorIndex;
					}

					// Check for format specifiers within the braces
					size_t end = fmt.find(L"}", pos);
					if (end != std::wstring::npos) {
						std::wstring formatSpecifier = fmt.substr(pos, end - pos + 1);
						std::wcout << std::vformat(formatSpecifier, std::make_wformat_args(TypeConvert(arg)));
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

			(printArg(args), ...);
			std::wcout << fmt.substr(start) << std::endl;
		}
		catch (const std::exception& e) {
			throw std::runtime_error("Failed to format string: " + std::string(e.what()));
		}
#endif
	}
//	void Print(const std::vector<Color>& colors, std::wstring fmt, const auto&... args)
//	{
//#if defined(_DEBUG) || defined(_CONSOLE)
//		try {
//			std::scoped_lock lock(cout_mutex);
//			HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//			if (hConsole == INVALID_HANDLE_VALUE) {
//				throw std::runtime_error("Failed to get console handle");
//			}
//			size_t start = 0;
//			std::wregex  re(L"\\{(.*?)\\}");
//			auto it = std::wsregex_iterator(fmt.begin(), fmt.end(), re);
//			size_t colorIndex = 0;
//			auto printArg = [&](const auto& arg) {
//				if (it != std::wsregex_iterator()) {
//					std::wcout << fmt.substr(start, it->position() - start);
//					if (colorIndex < colors.size()) {
//						if (!SetConsoleTextAttribute(hConsole, static_cast<WORD>(colors[colorIndex]))) {
//							throw std::runtime_error("Failed to set console text attribute");
//						}
//						++colorIndex;
//					}
//					std::wcout << std::vformat(it->str(), std::make_wformat_args(TypeConvert(arg)));
//					if (!SetConsoleTextAttribute(hConsole, static_cast<WORD>(Color::White))) {
//						throw std::runtime_error("Failed to set console text attribute");
//					}
//					start = it->position() + it->length();
//					++it;
//				}
//				else {
//					std::wcout << fmt.substr(start);
//				}
//			};
//			(printArg(args), ...);
//			std::wcout << fmt.substr(start) << std::endl;
//		}
//		catch (const std::exception& e) {
//			throw std::runtime_error("Failed to format string: " + std::string(e.what()));
//		}
//#endif
//	}
}

#endif // _CONSOLE_H