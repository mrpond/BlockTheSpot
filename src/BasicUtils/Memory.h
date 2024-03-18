#ifndef _MEMORY_H
#define _MEMORY_H

#include <string_view>
#include <initializer_list>
#include <vector>

namespace Memory {
    bool Read(void* address, void* buffer, size_t size);
    bool Write(void* address, const void* data, size_t size);
 
    bool Write(void* address, const std::string_view& data);
    bool Write(void* address, const std::wstring_view& data);
    bool Write(void* address, const std::initializer_list<uint8_t>& data);
    bool Write(void* address, const std::vector<uint8_t>& data);
    
}

#endif //_MEMORY_H
