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
struct Array2 {
    Array2() = default;
    Array2(u32 width, u32 height, Span<T> items)
        : w(width)
        , h(height)
        , items(items.raw_data())
    {
        ASSERT(items.size() == width * height);
    }

    s32 w;
    s32 h;
    T* items;

    size_t count() const { return w * h; }

    T& get(s32 x, s32 y)
    {
        ASSERT(x >= 0 && x < this->w && y >= 0 && y < this->h);
        return get_flat((y * this->w) + x);
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
        return x >= 0 && x < w && y >= 0 && y < h;
    }

    T get_if_exists(s32 x, s32 y, T default_value) const
    {
        if (contains_coordinate(x, y))
            return items[(y * w) + x];

        return default_value;
    }

    void set(s32 x, s32 y, T value)
    {
        get(x, y) = value;
    }

    void fill(T const& value)
    {
        fillMemory<T>(items, value, w * h);
    }

    void fill_region(Rect2I const& region, T const& value)
    {
        ASSERT(Rect2I(0, 0, w, h).contains(region));

        for (s32 y = region.y(); y < region.y() + region.height(); y++) {
            // Set whole rows at a time
            fillMemory<T>(items + (y * w) + region.x(), value, region.width());
        }
    }
};
