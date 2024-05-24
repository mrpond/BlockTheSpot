#ifndef _LOGGER_H
#define _LOGGER_H

#include <string_view>
#include "Utils.h"

namespace Logger
{
    enum class LogLevel { Info, Error, Debug };
    void Init(std::wstring_view file, bool enable);
    void Log(std::wstring_view message, LogLevel level);
    bool HasError();
}

using Logger::LogLevel;
using Logger::Log;

#define LogInfo(message, ...) Log(Utils::FormatString(message, __VA_ARGS__), LogLevel::Info)
#define LogError(message, ...) Log(Utils::FormatString(message, __VA_ARGS__), LogLevel::Error)

#if defined(_DEBUG)
#define LogDebug(message, ...) Log(Utils::FormatString(message, __VA_ARGS__), LogLevel::Debug)
#else
#define LogDebug(message, ...)
#endif

#endif //_LOGGER_H