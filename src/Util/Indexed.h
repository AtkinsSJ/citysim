/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

template<typename T>
class Indexed {
public:
    Indexed(u32 index, T& value)
        : m_index(index)
        , m_value(value)
    {
    }

    // FIXME: Temporary, until we consistently use unsigned types for indexes
    Indexed(s32 index, T& value)
        : m_index(index)
        , m_value(value)
    {
    }

    u32 index() const { return m_index; }
    T& value() { return m_value; }
    T const& value() const { return m_value; }

private:
    u32 m_index;
    T& m_value;
};
