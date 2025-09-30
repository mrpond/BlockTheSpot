#pragma once
#include <atomic>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <locale>
#include <mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <format>
#include <Windows.h>

namespace lg {
    template<typename CharT>
    class logger {
    public:
        using string_t = std::basic_string<CharT>;
        using view_t = std::basic_string_view<CharT>;
        using ostream_t = std::basic_ostream<CharT>;
        using ofstream_t = std::basic_ofstream<CharT>;
    
        enum class Level { TRACE, DEBUG, INFO, WARN, ERR, FATAL };
    
        static logger& instance() {
            static logger inst;
            return inst;
        }
    
        void init(const string_t& file_path = {}, bool enable = false) {
            m_enabled = enable; m_error_flag = false;
            try {
                std::locale loc("en_US.UTF-8");
                std::locale::global(loc);
                m_file.imbue(loc);
#ifndef NDEBUG
				initialize_console(loc);
#endif
            } catch (...) {}

            if (m_enabled && !file_path.empty()) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_file.open(file_path, std::ios::out | std::ios::trunc);
                if ((m_file_opened = m_file.is_open())) {
                    m_file << std::unitbuf;
                }
            }
        }
    
        bool is_enabled() const noexcept { return m_enabled; }
        bool has_error() const noexcept { return m_error_flag; }

        template<typename First, typename... Rest>
        void log(Level lvl, First&& first, Rest&&... rest) {
        #ifdef NDEBUG
            if (lvl == Level::TRACE || lvl == Level::DEBUG) return;
        #endif
            if (lvl < m_level) return;
            if (lvl >= Level::ERR) m_error_flag = true;

            using other_view = std::basic_string_view< std::conditional_t<std::is_same_v<CharT, char>, wchar_t, char>>;
            static_assert(!std::is_convertible_v<First, other_view>, "Error: string-literal type does not match logger<CharT>!");

            string_t msg;
            if constexpr (std::is_convertible_v<First, view_t>) {
                view_t fmt = std::forward<First>(first);
                if constexpr (sizeof...(Rest) > 0) {
                    if constexpr (std::is_same_v<CharT, wchar_t>) {
                        msg = std::vformat(fmt, std::make_wformat_args(rest...));
                    } else {
                        msg = std::vformat(fmt, std::make_format_args(rest...));
                    }
                } else {
                    msg = string_t(fmt);
                }
            } else {
                if constexpr (std::is_same_v<CharT, wchar_t>) {
                    msg = std::format(L"{}", std::forward<First>(first));
                } else {
                    msg = std::format("{}", std::forward<First>(first));
                }
            }

            write_and_flush(lvl, msg);
        }
    
    private:
        void write_and_flush(Level lvl, const string_t& msg) {
            if (m_enabled && m_file_opened) {
                auto now = std::chrono::system_clock::now();
                auto now_s = std::chrono::time_point_cast<std::chrono::seconds>(now);
                string_t ts;
                if constexpr (std::is_same_v<CharT, wchar_t>) {
                    ts = std::format(L"{:%Y-%m-%d %H:%M:%S}", now_s);
                } else {
                    ts = std::format("{:%Y-%m-%d %H:%M:%S}", now_s);
                }
                std::scoped_lock lock(m_mutex);
                m_file << make_line(level_code(lvl), msg, ts);
            }
#ifndef NDEBUG
            if (lvl != Level::INFO) {
                get_stream() << apply_color(make_line(level_code(lvl), msg), lvl);
            }
#endif
        }

        static ostream_t& get_stream() {
            if constexpr (std::is_same_v<CharT,wchar_t>) return std::wcerr;
            else return std::cerr;
        }

        static string_t make_line(const string_t& code, const string_t& msg, const string_t& ts = {}) {
            if constexpr (std::is_same_v<CharT, wchar_t>) {
                return ts.empty()
                    ? std::vformat(L"[{}] {}\n", std::make_wformat_args(code, msg))
                    : std::vformat(L"{} | {} | {}\n", std::make_wformat_args(ts, code, msg));
            } else {
                return ts.empty()
                    ? std::format("[{}] {}\n", code, msg)
                    : std::format("{} | {} | {}\n", ts, code, msg);
            }
        }

        static string_t level_code(Level lvl) {
            static constexpr const char* codes_n[] = {"TRC","DBG","INF","WRN","ERR","FTL"};
            static constexpr const wchar_t* codes_w[] = {L"TRC",L"DBG",L"INF",L"WRN",L"ERR",L"FTL"};
            if constexpr (std::is_same_v<CharT,wchar_t>) return codes_w[size_t(lvl)];
            else return codes_n[size_t(lvl)];
        }

#ifndef NDEBUG
        static string_t apply_color(const string_t& line, Level lvl) {
            auto s = line, c = color_code(lvl), r = reset_code();
            auto pos_open = s.find('['), pos_close = s.find(']', pos_open+1);
            if (pos_open == string_t::npos || pos_close == string_t::npos) return s;
            auto p1 = pos_open + 1;
            return s.replace(p1, pos_close - p1, c + s.substr(p1, pos_close - p1) + r);
        }
    
        static string_t color_code(Level lvl) {
            static constexpr const char* cols_n[] = {"\x1b[37m","\x1b[34m","\x1b[32m","\x1b[33m","\x1b[31m","\x1b[35m"};
            static constexpr const wchar_t* cols_w[] = {L"\x1b[37m",L"\x1b[34m",L"\x1b[32m",L"\x1b[33m",L"\x1b[31m",L"\x1b[35m"};
            if constexpr (std::is_same_v<CharT,wchar_t>) return cols_w[size_t(lvl)];
            else return cols_n[size_t(lvl)];
        }
    
        static string_t reset_code() {
            if constexpr (std::is_same_v<CharT,wchar_t>) return L"\x1b[0m";
            else return "\x1b[0m";
        }

        static void enable_vt_mode() noexcept {
            if (HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); h != INVALID_HANDLE_VALUE) {
                if (DWORD mode; GetConsoleMode(h, &mode)) {
                    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                }
            }
        }

        static void initialize_console(const std::locale& loc) {
            if (AllocConsole()) {
                FILE* f = nullptr;
                if (_wfreopen_s(&f, L"CONIN$",  L"r", stdin)  != 0)
                    MessageBoxW(0, L"Failed to redirect standard input",  L"Error", 0);
                if (_wfreopen_s(&f, L"CONOUT$", L"w", stdout) != 0)
                    MessageBoxW(0, L"Failed to redirect standard output", L"Error", 0);
                if (_wfreopen_s(&f, L"CONOUT$", L"w", stderr) != 0)
                    MessageBoxW(0, L"Failed to redirect standard error",  L"Error", 0);

                SetConsoleOutputCP(CP_UTF8);
                SetConsoleCP(CP_UTF8);
                enable_vt_mode();

                if constexpr (std::is_same_v<CharT, wchar_t>) {
                    std::wcin.imbue(loc);
                    std::wcout.imbue(loc);
                    std::wcerr.imbue(loc);
                } else {
                    std::cin.imbue(loc);
                    std::cout.imbue(loc);
                    std::cerr.imbue(loc);
                }
            }
        }
#endif

        bool m_enabled = false;
        bool m_error_flag = false;
        bool m_file_opened = false;
        Level m_level;
        ofstream_t m_file;
        std::mutex m_mutex;
    };
}

using LoggerA = lg::logger<char>;
using LoggerW = lg::logger<wchar_t>;

#ifndef LOG_INIT
#define LOG_INIT(file_path, enable) LoggerA::instance().init(file_path, enable)
#endif

#ifndef WLOG_INIT
#define WLOG_INIT(file_path, enable) LoggerW::instance().init(file_path, enable)
#endif

#define LOG_HAS_ERROR()  LoggerA::instance().has_error()
#define WLOG_HAS_ERROR() LoggerW::instance().has_error()

#ifndef NDEBUG
    #define LOG_TRACE(fmt, ...) LoggerA::instance().log(LoggerA::Level::TRACE, fmt, ##__VA_ARGS__)
    #define WLOG_TRACE(fmt, ...) LoggerW::instance().log(LoggerW::Level::TRACE, fmt, ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...) LoggerA::instance().log(LoggerA::Level::DEBUG, fmt, ##__VA_ARGS__)
    #define WLOG_DEBUG(fmt, ...) LoggerW::instance().log(LoggerW::Level::DEBUG, fmt, ##__VA_ARGS__)
#else
    #define LOG_TRACE(fmt, ...)
    #define WLOG_TRACE(fmt, ...)
    #define LOG_DEBUG(fmt, ...)
    #define WLOG_DEBUG(fmt, ...)
#endif

#define LOG_INFO(fmt, ...)  LoggerA::instance().log(LoggerA::Level::INFO, fmt, ##__VA_ARGS__)
#define WLOG_INFO(fmt, ...) LoggerW::instance().log(LoggerW::Level::INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LoggerA::instance().log(LoggerA::Level::WARN, fmt, ##__VA_ARGS__)
#define WLOG_WARN(fmt, ...) LoggerW::instance().log(LoggerW::Level::WARN, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) LoggerA::instance().log(LoggerA::Level::ERR, fmt, ##__VA_ARGS__)
#define WLOG_ERROR(fmt, ...)LoggerW::instance().log(LoggerW::Level::ERR, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) LoggerA::instance().log(LoggerA::Level::FATAL, fmt, ##__VA_ARGS__)
#define WLOG_FATAL(fmt, ...)LoggerW::instance().log(LoggerW::Level::FATAL, fmt, ##__VA_ARGS__)