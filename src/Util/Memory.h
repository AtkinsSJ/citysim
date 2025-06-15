/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Maths.h"
#include <cstring> // For memset

#define KB(x) ((x) * 1024)
#define MB(x) (KB(x) * 1024)
#define GB(x) (MB(x) * 1024)

#define ArrayCount(a) (sizeof(a) / sizeof(a[0]))
#define ArrayCountS(a) ((int)(ArrayCount(a)))

struct Blob {
    smm size;
    u8* memory;
};
inline Blob makeBlob(smm size, u8* memory)
{
    Blob result = {};

    result.size = size;
    result.memory = memory;

    return result;
}

// Allocates directly from the OS, not from an arena
u8* allocateRaw(smm size);
void deallocateRaw(void* memory);

template<typename T>
void copyMemory(T const* source, T* dest, smm length)
{
    memcpy(dest, source, length * sizeof(T));
}

template<typename T>
void fillMemory(T* memory, T value, smm length)
{
    T* currentElement = memory;
    smm remainingBytes = length * sizeof(T);

    // NB: The loop below works for a fractional number of Ts, but remainingBytes
    // is always a whole number of them! It's kind of weird. Could possibly make
    // this faster by restricting it to whole Ts.

    while (remainingBytes > 0) {
        smm toCopy = min<smm>(remainingBytes, sizeof(T));
        memcpy(currentElement, &value, toCopy);
        remainingBytes -= toCopy;
        currentElement++;
    }
}

template<>
inline void fillMemory<s8>(s8* memory, s8 value, smm length)
{
    memset(memory, value, length);
}

template<>
inline void fillMemory<u8>(u8* memory, u8 value, smm length)
{
    memset(memory, value, length);
}

template<typename T>
bool isMemoryEqual(T* a, T* b, smm length = 1)
{
    // Shortcut if we're comparing memory with itself
    return (a == b)
        || (memcmp(a, b, sizeof(T) * length) == 0);
}
