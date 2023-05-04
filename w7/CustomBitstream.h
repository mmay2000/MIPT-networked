#pragma once
#include <cstring>
#include <cstdint>

#include <iostream>

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

    void write_packing_uint32(uint32_t v)
    {
        if (v < 128)
        {
            uint8_t val = v << 1;
            write<uint8_t>(val);
        }
        else
        {
            if (v < 16384)
            {
                uint16_t val = (v << 2) + 1;
                write<uint16_t>(val);
                /*int arr[16] = {0};
                for (int i = 0; i < 16; ++i)
                {
                    arr[i] = val % 2;
                    val /= 2;
                    std::cout << arr[i] << " ";
                }*/
            }
            else
            {
                if (v < 1073741824)
                {
                    uint32_t val = (v << 2) + 3;
                    write<uint32_t>(val);

                }
            }
        }
        //printf("Packing error");
    }



    uint32_t read_packing_uint32()
    {
        uint8_t first_byte = *(uint8_t*)(ptr + offsetRead);
        uint8_t check = first_byte;
        int arr[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

        for (int i = 0; i < 8; ++i)
        {
            arr[i] = check % 2;
            check /= 2;
           // std::cout << arr[i] <<" ";
        }

        if (arr[0] == 0)
        {
            offsetRead += sizeof(uint8_t);
            return (uint32_t)(first_byte >> 1);
        }
        else if (arr[1] == 0)
        {
            uint32_t val = (uint32_t)((*((uint16_t*)(ptr + offsetRead)) - 1) >> 2);
            offsetRead += sizeof(uint16_t);
            return val;
        }
            
        else
        {
            uint32_t val = (uint32_t)(((*(uint32_t*)(ptr + offsetRead)) - 3) >> 2);
            offsetRead += sizeof(uint32_t);
            return val;
        }
        
        //printf("Unpacking error");
        return NULL;
    }

private:
    uint8_t* ptr;
    uint32_t offsetWrite = 0;
    uint32_t offsetRead = 0;
};

