/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "RawAllocator.h"

RawAllocator& RawAllocator::the()
{
    static RawAllocator* s_instance = new RawAllocator;
    return *s_instance;
}

Span<u8> RawAllocator::allocate_internal(size_t size)
{
    ASSERT(size < 1_GB); // Something is very wrong if we're trying to allocate an entire gigabyte for something!

    u8* result = static_cast<u8*>(calloc(size, 1));
    ASSERT(result != nullptr); // calloc() failed!!! I don't think there's anything reasonable we can do here.
    return { size, result };
}

void RawAllocator::deallocate_internal(Span<u8> data)
{
    free(data.raw_data());
}
