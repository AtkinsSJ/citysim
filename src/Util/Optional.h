/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>
#include <Util/Basic.h>

template<typename T>
class [[nodiscard]] Optional {
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

    Optional(Optional&& other)
        : m_has_value(other.has_value())
    {
        if (other.has_value())
            m_value = other.release_value();
    }

    Optional(Optional const& other)
        : m_has_value(other.has_value())
    {
        if (other.has_value())
            m_value = other.value();
    }

    Optional& operator=(Optional&& other)
    {
        if (m_has_value)
            release_value();
        if (other.has_value()) {
            m_has_value = true;
            m_value = other.release_value();
        }
        return *this;
    }

    Optional& operator=(Optional const& other)
    {
        if (this == &other)
            return *this;

        if (m_has_value)
            release_value();
        if (other.has_value()) {
            m_has_value = true;
            m_value = other.value();
        }
        return *this;
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
class [[nodiscard]] Optional<T&> {
public:
    Optional() = default;

    Optional(T& value)
        : m_value(&value)
    {
    }

    ~Optional() = default;

    bool has_value() const { return m_value != nullptr; }

    T const& value() const
    {
        ASSERT(m_value);
        return *m_value;
    }

    T& value()
    {
        ASSERT(m_value);
        return *m_value;
    }

    T const* operator->() const { return m_value; }
    T* operator->() { return m_value; }

    T& release_value()
    {
        ASSERT(m_value);
        auto& result = *m_value;
        m_value = nullptr;
        return result;
    }

    T value_or(T alternative) const
    {
        if (has_value())
            return value();
        return alternative;
    }

    bool operator==(Optional const& other) const
    {
        return m_value == other.m_value;
    }

    bool operator==(T const& other) const
    {
        return m_value == other;
    }

private:
    T* m_value { nullptr };
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
