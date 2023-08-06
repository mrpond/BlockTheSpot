#ifndef _MEMORY_H
#define _MEMORY_H

#include <Windows.h>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace Memory {
    template<typename T>
    bool Read(LPVOID address, T& buffer, std::size_t bufferSize = -1)
    {
        if (address == nullptr)
            throw std::runtime_error("Invalid address");

        if (bufferSize == static_cast<std::size_t>(-1))
            bufferSize = sizeof(buffer); // error

        DWORD oldProtect = 0;
        if (!VirtualProtect(address, bufferSize, PAGE_EXECUTE_READWRITE, &oldProtect))
            throw std::runtime_error("Failed to set memory protection for reading");

        std::memcpy(static_cast<void*>(&buffer), address, bufferSize);
    
        if (!VirtualProtect(address, bufferSize, oldProtect, &oldProtect))
            throw std::runtime_error("Failed to restore memory protection after reading");
        return true;
    }
    
    template <typename T>
    bool Write(LPVOID address, const T& buffer, std::size_t bufferSize = -1)
    {
        if (address == nullptr)
            throw std::runtime_error("Invalid address");

        const void* data = nullptr;

        if (bufferSize == static_cast<std::size_t>(-1))
        {
            if constexpr (std::is_integral_v<T>) {
                data = &buffer;
                bufferSize = sizeof(buffer);
            }
            else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring> || std::is_same_v<T, std::string_view> || std::is_same_v<T, std::wstring_view>) {
                data = buffer.data();
                bufferSize = buffer.size() * sizeof(typename T::value_type);
            }
            else if constexpr (std::is_same_v<T, const char*>)
            {
                bufferSize = std::strlen(buffer);
                data = buffer;
            }
            else if constexpr (std::is_same_v<T, const wchar_t*>) {
                bufferSize = std::wcslen(buffer);
                data = buffer;
            }
            else if constexpr (std::is_same_v<T, std::vector<typename T::value_type>>) {
                data = buffer.data();
                bufferSize = buffer.size() * sizeof(typename T::value_type);
            }
            else {
                throw std::runtime_error("Unsupported type");
            }
        }
        else
        {
            data = &buffer;
        }

        DWORD oldProtect = 0;
        if (!VirtualProtect(address, bufferSize, PAGE_EXECUTE_READWRITE, &oldProtect))
            throw std::runtime_error("Failed to set memory protection for writing");

        std::memcpy(address, data, bufferSize);

        if (!VirtualProtect(address, bufferSize, oldProtect, &oldProtect))
            throw std::runtime_error("Failed to restore memory protection after writing");

        return true;
    }

    template <typename T, typename U>
    std::size_t GetMemberFunctionOffset(U T::* member_ptr) {
        return reinterpret_cast<std::size_t>(&(((T*)nullptr)->*member_ptr));
    }
}

#endif //_MEMORY_H
