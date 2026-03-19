/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Memory.h>
#include <Util/Rectangle.h>

template<typename T>
class Array2 {
public:
    Array2() = default;
    Array2(u32 width, u32 height, Span<T> items)
        : items(items.raw_data())
        , m_width(width)
        , m_height(height)
    {
        ASSERT(items.size() == width * height);
    }

    T* items;

    u32 width() const { return m_width; }
    u32 height() const { return m_height; }
    u32 count() const { return m_width * m_height; }

    T& get(s32 x, s32 y)
    {
        ASSERT(x >= 0 && x < m_width && y >= 0 && y < m_height);
        return get_flat((y * m_width) + x);
    }
    T const& get(s32 x, s32 y) const
    {
        return const_cast<Array2*>(this)->get(x, y);
    }

    T& get_flat(size_t index)
    {
        ASSERT(index < count());
        return items[index];
    }
    T const& get_flat(size_t index) const
    {
        return const_cast<Array2*>(this)->get_flat(index);
    }

    bool contains_coordinate(s32 x, s32 y) const
    {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    }

    T get_if_exists(s32 x, s32 y, T default_value) const
    {
        if (contains_coordinate(x, y))
            return items[(y * m_width) + x];

        return default_value;
    }

    void set(s32 x, s32 y, T value)
    {
        get(x, y) = value;
    }

    void fill(T const& value)
    {
        fillMemory<T>(items, value, m_width * m_height);
    }

    void fill_region(Rect2I const& region, T const& value)
    {
        ASSERT(Rect2I(0, 0, m_width, m_height).contains(region));

        for (s32 y = region.y(); y < region.y() + region.height(); y++) {
            // Set whole rows at a time
            fillMemory<T>(items + (y * m_width) + region.x(), value, region.width());
        }
    }

private:
    u32 m_width { 0 };
    u32 m_height { 0 };
};
