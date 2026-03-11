/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/StringBase.h>

namespace Detail {

class StringBufferBase : public StringBase {
public:
    explicit StringBufferBase(char* buffer, size_t capacity)
        : StringBase(buffer, 0)
        , m_capacity(capacity)
    {
    }

    void append(StringBase const& other);
    void clear();

protected:
    size_t m_capacity;
};

}

template<size_t Capacity>
class StringBuffer : public Detail::StringBufferBase {
public:
    StringBuffer()
        : StringBufferBase(m_buffer, Capacity)
    {
    }

private:
    char m_buffer[Capacity] {};
};
