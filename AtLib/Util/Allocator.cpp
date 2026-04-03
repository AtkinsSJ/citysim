/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Allocator.h"
#include <Util/Memory.h>
#include <Util/String.h>

String Allocator::allocate_string(StringView input)
{
    auto character_data = allocate_multiple<char>(input.length());
    copy_memory(input.raw_pointer_to_characters(), character_data.raw_data(), input.length());
    return String { character_data.raw_data(), input.length() };
}

Blob Allocator::allocate_blob(size_t size)
{
    return Blob { size, allocate_internal(size).raw_data() };
}

Blob Allocator::allocate_blob(Blob const& source)
{
    auto result = allocate_blob(source.size());
    copy_memory(source.data(), result.writable_data(), source.size());
    return result;
}

void Allocator::deallocate(Blob& blob)
{
    deallocate_internal({ blob.size(), blob.writable_data() });
    blob = {};
}

void Allocator::deallocate(String& string)
{
    deallocate_internal({ string.length(), reinterpret_cast<u8*>(const_cast<char*>(string.raw_pointer_to_characters())) });
    string = {};
}
