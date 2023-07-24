#ifndef _LOGGER_H
#define _LOGGER_H

#include <string_view>

namespace Logger
{
    enum class LogLevel { Info, Error };
    void Init(std::wstring_view file, bool enable);
    void Close();
    void Log(std::wstring_view message, LogLevel level);
}

#endif //_LOGGER_H