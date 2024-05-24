#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "Logger.h"
#include "Utils.h"
#include <fstream>
#include <chrono>
#include <mutex>
#include <locale>
#include <codecvt>

namespace Logger {
    std::wofstream file;
    bool log_enabled = false;
    bool has_error = false;

    void Init(std::wstring_view log_file, bool enable_logging)
    {
        log_enabled = enable_logging;
        if (log_enabled) {
            file.open(log_file.data(), std::ios::out | std::ios::trunc);
            if (!file.is_open()) {
                PrintError(L"Failed to open log file.");
            }
            else {
                file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));
            }
        }
    }

    std::wstring GetLevelInfo(LogLevel level)
    {
        switch (level) {
            case LogLevel::Info: return L"INFO";
            case LogLevel::Error: return L"ERROR";
            case LogLevel::Debug: return L"DEBUG";
            default: return L"UNKN";
        }
    }

    void Log(std::wstring_view message, LogLevel level)
    {
        if (level == LogLevel::Error) {
            has_error = true;
            PrintError(L"{}", message);
        }
        
        if (level == LogLevel::Debug) {
            PrintInfo(L"{}", message);
        }

        if (!log_enabled) return;

        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);

        if (file.is_open()) {
            auto now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::wstringstream ss;
            ss << std::put_time(std::localtime(&now_time), L"%Y-%m-%d %H:%M:%S") << " | " << std::setw(5) << std::left << GetLevelInfo(level) << " | " << message << "\n";
            file << ss.str();
            file.flush();
        }
    }

    bool HasError()
    {
        return has_error;
    }
}
