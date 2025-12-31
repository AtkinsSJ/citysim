/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StringBuffer.h"
#include <Util/Memory.h>

namespace Detail {

void StringBufferBase::append(StringBase const& other)
{
    auto new_length = length() + other.length();
    ASSERT(new_length <= m_capacity);
    copy_memory(other.raw_pointer_to_characters(), mutable_chars() + m_length, other.length());
    m_length += other.length();
}

void StringBufferBase::clear()
{
    m_length = 0;
    fill_memory<char>(mutable_chars(), 0, m_capacity);
}

}
