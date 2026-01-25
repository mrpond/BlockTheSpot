#include "log_thread.h"
#include <cstdio>

constexpr size_t LOG_RING_SIZE = 256;   // number of messages
constexpr size_t LOG_MSG_SIZE = 256;   // bytes per message
constexpr Log_level MAX_LOG_LEVEL = Log_level::DEBUG;
struct Log_entry
{
    Log_level level;
    char msg[LOG_MSG_SIZE];
};

struct Log_work
{
    HANDLE log_thread = nullptr;
    DWORD log_thread_id = 0;
    bool log_enable = false;
    Log_level log_level = Log_level::NONE;
    Log_entry buffer[LOG_RING_SIZE]{};
    size_t write = 0;
    size_t read = 0;
};

inline Log_work logger;

inline size_t ring_next(size_t idx) noexcept
{
    return (idx + 1) % LOG_RING_SIZE;
}

DWORD WINAPI log_apc_worker(LPVOID)
{
    for (;;)
    {
        // Sleep forever, but alertable
        SleepEx(INFINITE, TRUE);
    }
    return 0;
}

void init_log_thread()
{
    logger.log_level = static_cast<Log_level>(GetPrivateProfileIntA(
        "Log",
        "Level",
        static_cast<int>(Log_level::NONE),
        CONFIG_FILEA
    ));

    if (Log_level::NONE == logger.log_level) {
        return;
    }

    if (logger.log_level > MAX_LOG_LEVEL) {
        logger.log_level = MAX_LOG_LEVEL;
    }

    DeleteFileW(LOG_FILEW);
    if (nullptr == logger.log_thread) {
        logger.log_thread = CreateThread(
            nullptr,                // default security
            0,                      // default stack
            log_apc_worker,
            nullptr,                // parameter
            0,                      // run immediately
            &logger.log_thread_id
        );
    }
	log_info("Log thread initialized");
}

static inline HANDLE open_log_file()
{
    HANDLE h = CreateFileW(
        LOG_FILEW,
        FILE_APPEND_DATA,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,                 // create if missing
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (h != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(h, 0, nullptr, FILE_END);
    }

    return h;
}

void CALLBACK log_work(ULONG_PTR param)
{
    static HANDLE log_file = INVALID_HANDLE_VALUE;

    if (INVALID_HANDLE_VALUE == log_file)
    {
        log_file = open_log_file();

        if (INVALID_HANDLE_VALUE == log_file)
            return;
    }

   
    for (; logger.read != logger.write; )
    {
        const auto& entry = logger.buffer[logger.read];
        char line[LOG_MSG_SIZE + 32]{};
        const char* prefix = "";

        switch (entry.level)
        {
        case Log_level::DEBUG:        prefix = "[DEBUG] "; break;
        case Log_level::INFORMATION:  prefix = "[INFO ] "; break;
        default: break;
        }

        const int len = _snprintf_s(
            line,
            sizeof(line),
            _TRUNCATE,
            "%s%s\r\n",
            prefix,
            entry.msg
        );

        DWORD written = 0;
        if (FALSE == WriteFile(log_file, line, static_cast<DWORD>(len), &written, nullptr))
        {
            // File may have been deleted -> reopen
            CloseHandle(log_file);
            log_file = INVALID_HANDLE_VALUE;
            break;
        }

        logger.read = ring_next(logger.read);
    }
}

void log_message(Log_level level, const char* message)
{
    if (nullptr == message) {
        OutputDebugStringW(L"log_message message empty!\n");
        return;
    }

    if (Log_level::NONE == logger.log_level ||
        nullptr == logger.log_thread ||
        0 == logger.log_thread_id) {
        return;
    }

    if (level > logger.log_level ||
        level > MAX_LOG_LEVEL) {
        OutputDebugStringW(L"log_message log_level mismatch!\n");
        return;
    }

    const auto current = logger.write;
    const size_t next = ring_next(current);

    if (next == logger.read) {
		OutputDebugStringW(L"Logger buffer full, dropping log message\n");
        return;
    }

    logger.buffer[current].level = level;
    strncpy_s(
        logger.buffer[current].msg,
        LOG_MSG_SIZE,
        message,
        _TRUNCATE
    );
    logger.write = next;

    QueueUserAPC(log_work, logger.log_thread, 0);
}
