/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Forward.h>

// Note for later: variadic macros `#define(foo, ...)` expand the ... with `__VA_ARGS__`

#define GLUE_(a, b) a##b
#define GLUE(a, b) GLUE_(a, b)

#include "Util/Basic.h"

template<typename T>
inline bool equals(T a, T b)
{
    return isMemoryEqual<T>(&a, &b);
}

enum Alignment {
    ALIGN_LEFT = 1,
    ALIGN_H_CENTRE = 2,
    ALIGN_RIGHT = 4,
    ALIGN_EXPAND_H = 8,

    ALIGN_H = ALIGN_LEFT | ALIGN_H_CENTRE | ALIGN_RIGHT | ALIGN_EXPAND_H,

    ALIGN_TOP = 16,
    ALIGN_V_CENTRE = 32,
    ALIGN_BOTTOM = 64,
    ALIGN_EXPAND_V = 128,

    ALIGN_V = ALIGN_TOP | ALIGN_V_CENTRE | ALIGN_BOTTOM | ALIGN_EXPAND_V,

    ALIGN_CENTRE = ALIGN_H_CENTRE | ALIGN_V_CENTRE,
};

enum class Orientation {
    Horizontal,
    Vertical
};

//
// Colours
//
V4 color255(u8 r, u8 g, u8 b, u8 a);
V4 makeWhite();
V4 asOpaque(V4 color);
