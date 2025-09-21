/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>
#include <Util/Basic.h>

template<typename T>
class Optional {
public:
    Optional()
        : m_has_value(false)
        , m_blank {}
    {
    }

    Optional(T value)
        : m_has_value(true)
        , m_value(move(value))
    {
    }

    ~Optional()
    {
        if (m_has_value)
            m_value.~T();
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

    T value_or(T alternative) const
    {
        if (has_value())
            return value();
        return alternative;
    }

    bool operator==(Optional const& other) const
    {
        if (m_has_value != other.m_has_value)
            return false;
        if (m_has_value && other.m_has_value)
            return m_value == other.m_value;
        return true;
    }

    bool operator==(T const& other) const
    {
        if (!m_has_value)
            return false;
        return m_value == other;
    }

private:
    bool m_has_value { false };
    union {
        T m_value;
        u8 m_blank[sizeof(T)];
    };
};

template<typename T>
bool all_have_values(Optional<T> const& input)
{
    return input.has_value();
}

template<typename T, typename... TS>
bool all_have_values(Optional<T> const& first, Optional<TS> const&... rest)
{
    return first.has_value() && all_have_values(rest...);
}
