#pragma once
#include "loader.h"

void init_log_thread();

enum class Log_level : uint8_t {
	NONE = 0,
	INFORMATION,
	DEBUG
};

void log_message(Log_level level, const char* message);

// Inline wrapper for debug logging
inline void log_debug(const char* message)
{
	log_message(Log_level::DEBUG, message);
}

// Inline wrapper for debug logging
inline void log_info(const char* message)
{
	log_message(Log_level::INFORMATION, message);
}