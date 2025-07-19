/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>

template<typename T>
class Optional {
public:
    Optional() = default;
    Optional(T value)
        : m_has_value(true)
        , m_value(move(value))
    {
    }

    bool has_value() const { return m_has_value; }

    T const& value() const
    {
        ASSERT(m_has_value);
        return m_value;
    }

    T& value()
    {
        ASSERT(m_has_value);
        return m_value;
    }

    T const& operator->() const { return value(); }
    T& operator->() { return value(); }

    T&& release_value()
    {
        ASSERT(m_has_value);
        m_has_value = false;
        return move(m_value);
    }

private:
    bool m_has_value { false };
    T m_value {};
};
