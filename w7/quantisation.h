#pragma once
#include "mathUtils.h"
#include <limits>

template<typename T>
T pack_float(float v, float lo, float hi, int num_bits)
{
	T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
	return range * ((clamp(v, lo, hi) - lo) / (hi - lo));
}

template<typename T>
float unpack_float(T c, float lo, float hi, int num_bits)
{
	T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
	return float(c) / range * (hi - lo) + lo;
}

template<typename T, int num_bits>
struct PackedFloat
{
	T packedVal;

	PackedFloat(float v, float lo, float hi) { pack(v, lo, hi); }
	PackedFloat(T compressed_val) : packedVal(compressed_val) {}

	void pack(float v, float lo, float hi) { packedVal = pack_float<T>(v, lo, hi, num_bits); }
	float unpack(float lo, float hi) { return unpack_float<T>(packedVal, lo, hi, num_bits); }
};

typedef PackedFloat<uint8_t, 4> float4bitsQuantized;

struct vec2
{
	float x;
	float y;
};

struct vec3
{
	float x;
	float y;
	float z;
};

template <typename T, int num_bits1, int num_bits2>
struct PackedVec2
{
	T packedval;

	PackedVec2(float v1, float v2, float lo, float hi) { pack(v1, v2, lo, hi); }
	PackedVec2(T compressed_val) : packedval(compressed_val) {}

	void pack(float v1, float v2, float lo, float hi) 
	{ 
		const T packedval1 = pack_float<T>(v1, lo, hi, num_bits1); 
		const T packedval2 = pack_float<T>(v2, lo, hi, num_bits2);

		packedval = packedval1 << num_bits2 | packedval2;
	}
	vec2 unpack(float lo, float hi) 
	{ 
		vec2 vect;
		vect.x = unpack_float<T>(packedval >> num_bits2, lo, hi, num_bits1);
		vect.y = unpack_float<T>(packedval & ((1 << num_bits2) - 1), lo, hi, num_bits2);
		return vect;
	}
};

template <typename T, int num_bits1, int num_bits2, int num_bits3>
struct PackedVec3
{
	T packedval;

	PackedVec3(float v1, float v2, float v3, float lo, float hi) { pack(v1, v2, v3, lo, hi); }
	PackedVec3(T compressed_val) : packedval(compressed_val) {}

	void pack(float v1, float v2, float v3, float lo, float hi)
	{
		const T packedval1 = pack_float<T>(v1, lo, hi, num_bits1);
		const T packedval2 = pack_float<T>(v2, lo, hi, num_bits2);
		const T packedval3 = pack_float<T>(v3, lo, hi, num_bits3);

		packedval = packedval1 << num_bits2 | packedval2;
		packedval = packedval << num_bits3 | packedval3;
	}
	vec2 unpack(float lo, float hi)
	{
		vec3 vect;
		vect.x = unpack_float<T>(packedval >> num_bits2, lo, hi, num_bits1);
		vect.y = unpack_float<T>((packedval >> num_bits3) & ((1 << num_bits2) - 1), lo, hi, num_bits2);
		vect.z = unpack_float<T>(packedval & ((1 << num_bits3) - 1), lo, hi, num_bits3);
		return vect;
	}
};


void* write_packing_uint32(uint32_t v)
{
    if (v < 128)
    {
        uint8_t val = v << 1;
        uint8_t* ptr = new uint8_t;
        *ptr = val;
        return ptr;
    }
    else
    {
        if (v < 16384)
        {
			uint16_t val = (v << 2) + 1;
            uint16_t* ptr = new uint16_t;
            *ptr = val;
            return ptr;
        }
        else
        {
            if (v < 1073741824)
            {
                uint32_t val = (v << 2) + 3;
                uint32_t* ptr = new uint32_t;
                *ptr = val;
                return ptr;
            }
            else
                return nullptr;
        }
    }

    return nullptr;
}



uint32_t read_packing_uint32(void* ptr)
{
    if (ptr == nullptr)
    {
        return NULL;
    }
    uint8_t first_byte = *(uint8_t*)ptr;
	uint8_t check = first_byte;
	int arr[8] = {0, 0, 0, 0, 0, 0, 0, 0};
	
	for (int i = 0; i < 8; ++i)
	{
		arr[i] = check % 2;
		//printf("%d", arr[i]);
		check /= 2;
	}

	if (arr[0] == 0)
		return (uint32_t)(first_byte >> 1);
    else if (arr[1] == 0)
        return (uint32_t)(( * ((uint16_t*)ptr) - 1) >> 2);
    else
        return (uint32_t)((( * (uint32_t*)ptr) - 3) >> 2);
}

/*
void check_uint32_compression()
{
	uint32_t a = 123;
	void* packed_int1 = write_packing_uint32(a);
	printf(" %d compressed and decompressed to %d\n", a, read_packing_uint32(packed_int1));

	a = 1083;
	void* packed_int2 = write_packing_uint32(a);
	printf(" %d compressed and decompressed to %d\n", a, read_packing_uint32(packed_int2));

	a = 10737482;
	void* packed_int3 = write_packing_uint32(a);
	printf(" %d compressed and decompressed to %d\n", a, read_packing_uint32(packed_int3));

	a = 1073741829;
	void* packed_int4 = write_packing_uint32(a);
	if (read_packing_uint32(packed_int4) == NULL)
		printf(" %d id uncompessiable\n", a);
}
*/