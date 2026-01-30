#include "log_thread.h"

constexpr size_t LOG_RING_SIZE = 512;   // number of messages
constexpr size_t LOG_MSG_SIZE = 256;   // bytes per message
constexpr Log_level MAX_LOG_LEVEL = Log_level::DEBUG;

constexpr size_t LOG_BULK_BUFFER_SIZE = 32 * 1024; // 32 KB

struct Log_entry
{
	Log_level level;
	char msg[LOG_MSG_SIZE];
};

struct Log_work
{
	HANDLE log_thread = nullptr;
	HANDLE timer = nullptr;
	HANDLE stop_event = nullptr;
	DWORD log_thread_id = 0;
	bool log_enable = false;
	Log_level log_level = Log_level::NONE;
	Log_entry buffer[LOG_RING_SIZE]{};
	size_t write = 0;
	size_t read = 0;
};

static inline Log_work logger;

static inline size_t ring_next(size_t idx) noexcept
{
	return (idx + 1) % LOG_RING_SIZE;
}

VOID CALLBACK log_work(ULONG_PTR param);
DWORD WINAPI log_apc_worker(LPVOID)
{
	HANDLE wait_handles[2] = {
		logger.timer,
		logger.stop_event
	};
	for (;;)
	{
		// Alertable wait on the timer
		DWORD wait = WaitForMultipleObjects(
			2,
			wait_handles,
			FALSE,
			INFINITE
		);
		if (WAIT_OBJECT_0 + 1 == wait)
		{
			break;
		}
		log_work(0);
	}
	return 0;
}

static inline void clear_log_file() noexcept
{
	HANDLE h = CreateFileW(
		LOG_FILEW,
		GENERIC_WRITE,             // need write access to truncate
		FILE_SHARE_READ,           // allow other readers
		nullptr,
		OPEN_ALWAYS,               // create if missing
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (h != INVALID_HANDLE_VALUE)
	{
		// Truncate file to 0
		SetFilePointer(h, 0, nullptr, FILE_BEGIN);
		SetEndOfFile(h);
		CloseHandle(h);
	}
}

static inline HANDLE reopen_log_file() noexcept
{
	HANDLE h = CreateFileW(
		LOG_FILEW,
		FILE_APPEND_DATA,
		FILE_SHARE_READ,
		nullptr,
		OPEN_ALWAYS,   // create if missing
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (h != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(h, 0, nullptr, FILE_END);
	}

	return h;
}

VOID CALLBACK log_work(ULONG_PTR param)
{
	static HANDLE log_file = INVALID_HANDLE_VALUE;
	static char bulk_buffer[LOG_BULK_BUFFER_SIZE];
	if (INVALID_HANDLE_VALUE == log_file ||
		INVALID_FILE_ATTRIBUTES == GetFileAttributesW(LOG_FILEW)) {
		if (INVALID_HANDLE_VALUE != log_file) {
			CloseHandle(log_file);
			log_file = INVALID_HANDLE_VALUE;
		}

		log_file = reopen_log_file();

		if (INVALID_HANDLE_VALUE == log_file) {
			OutputDebugStringW(L"log_work: reopen_log_file failed!\n");
			return;
		}
	}

	size_t bulk_used = 0;
	for (; logger.read != logger.write; )
	{
		const auto& entry = logger.buffer[logger.read];
		//char line[LOG_MSG_SIZE + 32]{};
		const char* prefix = "";

		switch (entry.level)
		{
		case Log_level::DEBUG:        prefix = "[DEBUG] "; break;
		case Log_level::INFORMATION:  prefix = "[INFO ] "; break;
		default: break;
		}
		constexpr size_t worst =
			8 + LOG_MSG_SIZE + 2; // prefix + msg + CRLF

		if (bulk_used + worst >= LOG_BULK_BUFFER_SIZE)
			break;

		const int len = _snprintf_s(
			bulk_buffer + bulk_used,
			LOG_BULK_BUFFER_SIZE - bulk_used,
			_TRUNCATE,
			"%s%s\r\n",
			prefix,
			entry.msg
		);

		if (len <= 0)
			break;

		bulk_used += static_cast<size_t>(len);
		logger.read = ring_next(logger.read);
	}

	if (0 == bulk_used)
		return;

	DWORD written = 0;
	if (FALSE == WriteFile(log_file, bulk_buffer, static_cast<DWORD>(bulk_used), &written, nullptr))
	{
		// File may have been deleted -> reopen
		CloseHandle(log_file);
		log_file = INVALID_HANDLE_VALUE;
		//break;
	}	
}

static inline void log_message(Log_level level, const char* message) noexcept
{
	if (nullptr == message) {
		OutputDebugStringW(L"log_message message empty!\n");
		return;
	}

	if (nullptr == logger.log_thread ||
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

	//QueueUserAPC(log_work, logger.log_thread, 0);
}

static inline void log_any_noop(const char* message) noexcept {}
static inline void log_debug_impl(const char* message) noexcept
{
	log_message(Log_level::DEBUG, message);
}
static inline void log_info_impl(const char* message) noexcept
{
	log_message(Log_level::INFORMATION, message);
}

void init_log_thread() noexcept
{
	logger.log_level = static_cast<Log_level>(GetPrivateProfileIntA(
		"Log",
		"Level",
		static_cast<int>(Log_level::NONE),
		CONFIG_FILEA
	));

	log_debug = &log_any_noop;
	log_info = &log_any_noop;
	if (Log_level::NONE == logger.log_level) {
		OutputDebugStringW(L"init_log_thread: log disable!\n");
		return;
	}

	log_info = &log_info_impl;
	if (Log_level::DEBUG == logger.log_level) {
		log_debug = &log_debug_impl;
	}

	if (logger.log_level > MAX_LOG_LEVEL) {
		logger.log_level = MAX_LOG_LEVEL;
	}

	if (!logger.timer) {
		logger.timer = CreateWaitableTimerW(nullptr, FALSE, L"Log Interval");
		if (!logger.timer) {
			OutputDebugStringW(L"init_log_thread: CreateWaitableTimerW fail!\n");
			return;
		}
	}
	if (!logger.stop_event) {
		logger.stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		if (!logger.timer) {
			OutputDebugStringW(L"init_log_thread: CreateEventW fail!\n");
			return;
		}
	}
	

	LARGE_INTEGER due{};
	due.QuadPart = -5'000'000LL;

	if (FALSE == SetWaitableTimer(
		logger.timer,
		&due,
		500,
		nullptr,
		nullptr,
		FALSE)) {
		OutputDebugStringW(L"init_log_thread: SetWaitableTimer fail!\n");
		return;
	}

	//DeleteFileW(LOG_FILEW);
	clear_log_file();

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
	log_info("init_log_thread: initialized");
}

void stop_log() noexcept
{
	if (Log_level::NONE == logger.log_level) {
		return;
	}

	if (logger.stop_event) {
		SetEvent(logger.stop_event);
	}

	if (logger.timer) {
		CancelWaitableTimer(logger.timer);
		CloseHandle(logger.timer);
	}

	if (logger.log_thread) {
		// Wait for thread to exit
		WaitForSingleObject(logger.log_thread, INFINITE);
		CloseHandle(logger.log_thread);
	}
}
