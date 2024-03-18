#include "Memory.h"
#include "Utils.h"

namespace Memory {
    bool Read(void* address, void* buffer, size_t size)
    {
        DWORD oldProtect;
        if (VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memcpy(buffer, address, size);
            VirtualProtect(address, size, oldProtect, &oldProtect);
            return true;
        }
        return false;
    }

    bool Write(void* address, const void* data, size_t size) 
    {
        DWORD oldProtect;
        if (VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memcpy(address, data, size);
            VirtualProtect(address, size, oldProtect, &oldProtect);
            return true;
        }
        return false;
    }

    bool Write(void* address, const std::string_view& data)
    {
        return Write(address, data.data(), data.size());
    }
    
    bool Write(void* address, const std::wstring_view& data)
    {
        return Write(address, data.data(), data.size());
    }

    bool Write(void* address, const std::initializer_list<uint8_t>& data)
    {
        return Write(address, data.begin(), data.size());
    }

    bool Write(void* address, const std::vector<uint8_t>& data)
    {
        return Write(address, data.data(), data.size());
    }
}