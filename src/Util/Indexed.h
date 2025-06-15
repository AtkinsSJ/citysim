/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Basic.h"

template<typename T>
struct Indexed {
    T value;
    s32 index;
};

template<typename T>
inline Indexed<T> makeNullIndexedValue()
{
    Indexed<T> result;
    result.value = nullptr;
    result.index = -1;

    return result;
}

template<typename T>
inline Indexed<T> makeIndexedValue(T value, s32 index)
{
    Indexed<T> result;
    result.value = value;
    result.index = index;

    return result;
}
