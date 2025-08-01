/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Forward.h>
#include <Util/Memory.h>
#include <Util/Rectangle.h>

template<typename T>
struct Array2 {
    s32 w;
    s32 h;
    T* items;

    T& get(s32 x, s32 y)
    {
        ASSERT(x >= 0 && x < this->w && y >= 0 && y < this->h);

        return this->items[(y * this->w) + x];
    }

    T getIfExists(s32 x, s32 y, T defaultValue)
    {
        T result = defaultValue;

        if (x >= 0 && x < this->w && y >= 0 && y < this->h) {
            result = this->items[(y * this->w) + x];
        }

        return result;
    }

    void set(s32 x, s32 y, T value)
    {
        this->get(x, y) = value;
    }
};

template<typename T>
Array2<T> makeArray2(s32 w, s32 h, T* items)
{
    Array2<T> result = {};

    result.w = w;
    result.h = h;
    result.items = items;

    return result;
}

template<typename T>
void fill(Array2<T>* array, T value)
{
    fillMemory<T>(array->items, value, array->w * array->h);
}

template<typename T>
void fillRegion(Array2<T>* array, Rect2I region, T value)
{
    ASSERT(Rect2I(0, 0, array->w, array->h).contains(region));

    for (s32 y = region.y; y < region.y + region.height(); y++) {
        // Set whole rows at a time
        fillMemory<T>(array->items + (y * array->w) + region.x, value, region.width());
    }
}
