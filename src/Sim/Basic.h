/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Colour.h>
#include <Gfx/Sprite.h>
#include <Util/Vector.h>
#include <flecs.h>

struct mod_basic {
    explicit mod_basic(flecs::world&);
};

// Components
struct PositionComponent {
    V2 position;
};

struct SpriteComponent {
    SpriteRef sprite;
    V2 size;
    Colour color;
};

// Singletons
struct VisibleTileBounds {
    Rect2I rect;
};
