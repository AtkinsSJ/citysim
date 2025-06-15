/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "../render.h"
#include "Forward.h"

namespace UI {

struct DrawableStyle;

struct Drawable {
    Drawable() { }
    Drawable(DrawableStyle* style)
        : style(style)
    {
    }

    void preparePlaceholder(RenderBuffer* buffer);
    void fillPlaceholder(Rect2I bounds);

    void draw(RenderBuffer* buffer, Rect2I bounds);

    DrawableStyle* style;

    union {
        DrawRectPlaceholder rectPlaceholder;
        DrawNinepatchPlaceholder ninepatchPlaceholder;
    };
};

}
