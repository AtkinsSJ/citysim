/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

template<typename T>
class Span {
public:
    Span(size_t size, T* items)
        : m_size(size)
        , m_items(items)
    {
    }

    Span() = default;

    size_t size() const { return m_size; }
    bool is_empty() const { return m_size == 0; }

    T* raw_data() const { return m_items; }

    T& first()
    {
        ASSERT(m_size > 0); // Index out of range!
        return m_items[0];
    }
    T const& first() const { return const_cast<Span&>(*this).first(); }

    T& last()
    {
        ASSERT(m_size > 0); // Index out of range!
        return m_items[m_size - 1];
    }
    T const& last() const { return const_cast<Span&>(*this).last(); }

    T& operator[](size_t index)
    {
        ASSERT(index < m_size);
        return m_items[index];
    }
    T const& operator[](size_t index) const { return const_cast<Span&>(*this)[index]; }

    template<typename Filter>
    bool all_are(Filter filter) const
    {
        for (int i = 0; i < m_size; ++i) {
            if (!filter(m_items[i]))
                return false;
        }
        return true;
    }

    template<typename Filter>
    bool any_are(Filter filter) const
    {
        for (int i = 0; i < m_size; ++i) {
            if (filter(m_items[i]))
                return true;
        }
        return false;
    }

    class Iterator {
    public:
        explicit Iterator(Badge<Span>, T* pointer)
            : m_pointer(pointer)
        {
        }

        T& operator*() { return *m_pointer; }
        T* operator->() { return m_pointer; }

        Iterator& operator++()
        {
            ++m_pointer;
            return *this;
        }

        Iterator operator++(int)
        {
            auto result = *this;
            ++(*this);
            return result;
        }

        bool operator==(Iterator const& other)
        {
            return m_pointer == other.m_pointer;
        }

        bool operator!=(Iterator const& other)
        {
            return m_pointer != other.m_pointer;
        }

    private:
        T* m_pointer;
    };

    Iterator begin() const
    {
        return Iterator({}, m_items);
    }

    Iterator end() const
    {
        return Iterator({}, m_items + m_size);
    }

private:
    size_t m_size;
    T* m_items;
};
