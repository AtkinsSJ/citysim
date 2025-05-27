#pragma once

// NB: This is inspired by https://github.com/tatewake/endian-template/blob/master/tEndian.h
// However, we require a *very small* subset of that! Just assignment to/from LE types from
// their native equivalents. So, I implemented it myself.

// Note that SDL_endian.h defines a lot of things itself, including assembly for byte-swapping,
// which is undoubtedly faster than this code. @Speed

inline bool cpuIsLittleEndian()
{
    u16 i = 1;

    return (*((u8*)&i) == 1);
}

template<typename T>
T reverseBytes(T input)
{
    T result;

    u8* inputBytes = (u8*)&input;
    u8* outputBytes = (u8*)&result;

    switch (sizeof(T)) {
    case 8: {
        outputBytes[0] = inputBytes[7];
        outputBytes[1] = inputBytes[6];
        outputBytes[2] = inputBytes[5];
        outputBytes[3] = inputBytes[4];
        outputBytes[4] = inputBytes[3];
        outputBytes[5] = inputBytes[2];
        outputBytes[6] = inputBytes[1];
        outputBytes[7] = inputBytes[0];
    } break;

    case 4: {
        outputBytes[0] = inputBytes[3];
        outputBytes[1] = inputBytes[2];
        outputBytes[2] = inputBytes[1];
        outputBytes[3] = inputBytes[0];
    } break;

    case 2: {
        outputBytes[0] = inputBytes[1];
        outputBytes[1] = inputBytes[0];
    } break;

    default: {
        ASSERT(false); // Only basic types are supported
    } break;
    }

    return result;
}

template<typename T>
struct LittleEndian {
    T data;

    //
    // SET
    //
    // Direct assignment
    LittleEndian(T value)
        : data(cpuIsLittleEndian() ? value : reverseBytes(value))
    {
    }

    LittleEndian()
        : data()
    {
    }

    //
    // GET
    //
    operator T() const { return cpuIsLittleEndian() ? data : reverseBytes(data); }
};

typedef u8 leU8; // Just for consistency
typedef LittleEndian<u16> leU16;
typedef LittleEndian<u32> leU32;
typedef LittleEndian<u64> leU64;

typedef s8 leS8; // Just for consistency
typedef LittleEndian<s16> leS16;
typedef LittleEndian<s32> leS32;
typedef LittleEndian<s64> leS64;

typedef LittleEndian<f32> leF32;
typedef LittleEndian<f64> leF64;
