/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Memory.h"
#include <cstdlib> // For calloc

u8* allocateRaw(smm size)
{
    ASSERT(size < GB(1)); // Something is very wrong if we're trying to allocate an entire gigabyte for something!

    u8* result = (u8*)calloc(size, 1);
    ASSERT(result != nullptr); // calloc() failed!!! I don't think there's anything reasonable we can do here.
    return result;
}

void deallocateRaw(void* memory)
{
    free(memory);
}
