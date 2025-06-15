/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>
#include <Util/Rectangle.h>
#include <Util/String.h>

struct Sprite {
    Asset* texture;
    Rect2 uv; // in (0 to 1) space
    s32 pixelWidth;
    s32 pixelHeight;
};

struct SpriteGroup {
    s32 count;
    Sprite* sprites;
};

struct SpriteRef {
    String spriteGroupName;
    s32 spriteIndex;

    Sprite* pointer;
    u32 pointerRetrievedTicks;
};
