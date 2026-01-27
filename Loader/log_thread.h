#pragma once
#include "loader.h"

void init_log_thread() noexcept;

enum class Log_level : uint8_t {
	NONE = 0,
	INFORMATION,
	DEBUG
};

using log_debug_t = void (*)(const char*) noexcept;
inline log_debug_t log_debug = nullptr;

using log_info_t = void (*)(const char*) noexcept;
inline log_info_t log_info = nullptr;

void stop_log() noexcept;