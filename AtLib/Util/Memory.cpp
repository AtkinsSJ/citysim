/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Memory.h"
#include <Util/Assert.h>
#include <cstdlib> // For calloc

u8* allocate_raw(size_t size)
{
    ASSERT(size < 1_GB); // Something is very wrong if we're trying to allocate an entire gigabyte for something!

    u8* result = static_cast<u8*>(calloc(size, 1));
    ASSERT(result != nullptr); // calloc() failed!!! I don't think there's anything reasonable we can do here.
    return result;
}

void deallocate_raw(void* memory)
{
    free(memory);
}
