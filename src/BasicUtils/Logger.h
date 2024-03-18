#ifndef _LOGGER_H
#define _LOGGER_H

#include <string_view>

namespace Logger
{
    enum class LogLevel { Info, Error };
    void Init(std::wstring_view file, bool enable);
    void Log(std::wstring_view message, LogLevel level);
    bool HasError();
}

using Logger::LogLevel;
using Logger::Log;

#endif //_LOGGER_H