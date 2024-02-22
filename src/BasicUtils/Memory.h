#ifndef _MEMORY_H
#define _MEMORY_H

#include <string_view>
#include <initializer_list>

namespace Memory {
    bool Read(void* address, void* buffer, size_t size);
    bool Write(void* address, const void* data, size_t size);
 
    bool Write(void* address, std::string_view& data);
    bool Write(void* address, std::initializer_list<uint8_t>& data);
    
}

#endif //_MEMORY_H
