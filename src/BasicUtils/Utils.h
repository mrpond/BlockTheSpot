#ifndef _UTILS_H
#define _UTILS_H

#include <functional>
#include <format>
#include <map>

namespace Utils
{
    std::string ToHexString(const std::vector<uint8_t>& byte_array, const bool insert_spaces = true);
    std::wstring ToHexWideString(const std::vector<uint8_t>& byte_array, const bool insert_spaces = true);

    std::string ToHexString(const uint8_t* data, size_t size, const bool insert_spaces = true);
    std::wstring ToHexWideString(const uint8_t* data, size_t size, const bool insert_spaces = true);
    
    std::string ConvertUInt8ArrayToString(std::uint8_t* data);
    std::wstring ConvertUInt8ArrayToWideString(std::uint8_t* data);

    std::string ToString(std::wstring_view wide_string);
    std::wstring ToString(std::string_view narrow_string);

    bool Contains(std::string_view str1, std::string_view str2, bool case_sensitive = false);
    bool Contains(std::wstring_view str1, std::wstring_view str2, bool case_sensitive = false);

    bool Equals(std::string_view str1, std::string_view str2, bool case_sensitive = false);
    bool Equals(std::wstring_view str1, std::wstring_view str2, bool case_sensitive = false);

	//Example 1: MeasureExecutionTime(std::bind(Function, arg1, arg2), name);
	//Example 2: MeasureExecutionTime([&]() { Function(arg1, arg2); }, name);
	void MeasureExecutionTime(std::function<void()> func, std::wstring_view name = {});
	using IniData = std::map<std::wstring, std::map<std::wstring, bool>>;
	IniData ReadIniFile(const std::wstring& fileName);
	void AppendIniFile(const std::wstring& fileName, IniData& data);

#ifndef NDEBUG
    void PrintSymbols(std::wstring_view module_name);
#endif

    void SetLocaleToUTF8();

    template<typename T>
    std::vector<std::uint8_t> ToBytes(const T& val)
    {
        struct is_string_type : std::disjunction<std::is_same<T, std::wstring>, std::is_same<T, std::wstring_view>, std::is_same<T, wchar_t*>, std::is_same<T, const wchar_t*>> {};
        if constexpr (is_string_type::value)
        {
            std::wstring_view str_view(val);
            return std::vector<std::uint8_t>(str_view.begin(), str_view.end());
        }
        else if constexpr (std::is_trivially_copyable_v<T>) {
            auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(val);
            return { bytes.begin(), bytes.end() };
        }
        else {
            throw std::runtime_error("Invalid type for ToBytes function");
        }
    }

    template<typename... Args>
    std::wstring FormatString(std::wstring_view fmt, const Args&... args)
    {
        try {
            return std::vformat(fmt, std::make_wformat_args(args...));
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to format string: " + std::string(e.what()));
        }
    }

    template<typename... Args>
    std::string FormatString(std::string_view fmt, const Args&... args)
    {
        try {
            return std::vformat(fmt, std::make_format_args(args...));
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Failed to format string: " + std::string(e.what()));
        }
    }
};

#endif // _UTILS_H