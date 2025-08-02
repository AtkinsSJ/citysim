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
    Indexed(s32 index, T value)
        : m_index(index)
        , m_value(value)
    {
    }

    s32 index() const { return m_index; }
    T& value() { return m_value; }
    T const& value() const { return m_value; }

private:
    s32 m_index;
    T m_value;
};
