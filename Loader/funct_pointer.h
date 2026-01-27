#pragma once
#include <cstdint>

template<typename T>
inline T get_funct_t(void* base, size_t offset)
{
    return *reinterpret_cast<T*>(
        reinterpret_cast<uintptr_t>(base) + offset
        );
}
