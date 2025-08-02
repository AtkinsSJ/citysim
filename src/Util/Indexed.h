/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

template<typename T>
struct Indexed {
    // FIXME: Temporary!
    Indexed() = default;

    Indexed(T value, s32 index)
        : value(value)
        , index(index)
    {
    }

    T value;
    s32 index;
};
