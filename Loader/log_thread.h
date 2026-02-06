#pragma once
#include "loader.h"

void init_log_thread() noexcept;

enum class Log_level : uint8_t {
	NONE = 0,
	INFORMATION,
	DEBUG
};

void log_any_noop(const char* message) noexcept;

using log_debug_t = void (*)(const char*) noexcept;
inline log_debug_t log_debug = log_any_noop;

using log_info_t = void (*)(const char*) noexcept;
inline log_info_t log_info = log_any_noop;
void stop_log() noexcept;