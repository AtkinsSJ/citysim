/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Util/Basic.h>
#include <Util/Optional.h>

enum class HAlign : u8 {
    Left,
    Centre,
    Right,
    Fill,
};

enum class VAlign : u8 {
    Top,
    Centre,
    Bottom,
    Fill,
};

struct Alignment {
    HAlign horizontal;
    VAlign vertical;

    Alignment()
        : horizontal(HAlign::Left)
        , vertical(VAlign::Top)
    {
    }

    Alignment(HAlign h, VAlign v)
        : horizontal(h)
        , vertical(v)
    {
    }

    Alignment(HAlign h)
        : horizontal(h)
        , vertical(VAlign::Top)
    {
    }

    Alignment(VAlign v)
        : horizontal(HAlign::Left)
        , vertical(v)
    {
    }

    static Optional<Alignment> read(LineReader&);

    static Alignment centre() { return { HAlign::Centre, VAlign::Centre }; }
    static Alignment fill() { return { HAlign::Fill, VAlign::Fill }; }

    bool operator==(Alignment const&) const = default;
};
