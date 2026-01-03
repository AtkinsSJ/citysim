/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Colour.h>
#include <Util/Array.h>

struct Palette {
    enum class Type : u8 {
        Fixed,
        Gradient,
    };
    Type type;
    s32 size;

    struct
    {
        Colour from;
        Colour to;
    } gradient;

    Array<Colour> paletteData;
};
