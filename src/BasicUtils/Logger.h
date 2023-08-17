#ifndef _LOGGER_H
#define _LOGGER_H

#pragma warning(disable: 4530)
#include <string_view>
#pragma warning(default: 4530)

namespace Logger
{
    enum class LogLevel { Info, Error };
    void Init(std::wstring_view file, bool enable);
    void Close();
    void Log(std::wstring_view message, LogLevel level);
}

#endif //_LOGGER_H