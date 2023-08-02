#include "pch.h"

#ifndef NDEBUG

#include <intrin.h>

DWORD WINAPI Debug(LPVOID lpParam)
{
    try
    {
        Utils::MeasureExecutionTime([&]() {
            //Utils::PrintSymbols(L"chrome_elf.dll");



            }, L"DEBUG");
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
#endif