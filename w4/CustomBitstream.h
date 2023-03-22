#pragma once
#include <cstring>
#include <cstdint>

class CustomBitstream
{
public:
    CustomBitstream(uint8_t* p) : ptr(p) {}
    template<typename T>
    void write(const T& data)
    {
        memcpy(ptr + offsetWrite, reinterpret_cast<const uint8_t*>(&data), sizeof(T));
        offsetWrite += sizeof(T);
    }

    template<typename T>
    void read(T& data)
    {
        memcpy(reinterpret_cast<uint8_t*>(&data), ptr + offsetRead, sizeof(T));
        offsetRead += sizeof(T);
    }

private:
    uint8_t* ptr;
    uint32_t offsetWrite = 0;
    uint32_t offsetRead = 0;
};

