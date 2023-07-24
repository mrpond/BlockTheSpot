#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include "Logger.h"
#include "Console.h"
#include <fstream>
#include <chrono>

#include <locale>
#include <codecvt>

using namespace Console;

namespace Logger
{
    std::wofstream file;
    std::wstringstream buffer;

    void Init(std::wstring_view log_file, bool log_enable)
    {
        if (log_enable)
        {
            file.open(log_file.data(), std::ios::out | std::ios::trunc);

            std::locale utf8_locale("en_US.UTF-8");
            file.imbue(utf8_locale);
            buffer.imbue(utf8_locale);

            if (!file.is_open()) {
                Print({ Color::Red }, L"[{}] Failed to open log file.", L"ERROR");
            }
        }
    }

    void Flush()
    {
        if (file.is_open())
        {
            file << buffer.str();
            buffer.str(L"");
            file.flush();
        }
    }

    void Close()
    {
        if (file.is_open())
        {
            Flush();
            file.close();
        }
    }

    std::wstring GetLevelInfo(LogLevel level)
    {
        switch (level) {
        case LogLevel::Info:
            return L"INFO";
        case LogLevel::Error:
            return L"ERROR";
        default:
            return L"UNKNOWN";
        }
    }

    void Log(std::wstring_view message, LogLevel level)
    {
        auto level_str = GetLevelInfo(level);

#ifndef NDEBUG
        if (level == LogLevel::Error) {
            Print({ Color::Red }, L"[{}] {}", level_str, message);
        }
#endif

        if (file.is_open())
        {
            auto now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::wostringstream ss;
            ss << std::put_time(std::localtime(&now_time), L"%Y-%m-%d %H:%M:%S");
            std::wstring time_str = ss.str();
            buffer << time_str << L" | " << level_str << L" | " << message << std::endl;
            Flush();
        }
    }
}