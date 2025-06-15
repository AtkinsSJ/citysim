/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "types.h"
#include <Util/Vector.h>

/**********************************************
    Colours
 **********************************************/

V4 color255(u8 r, u8 g, u8 b, u8 a)
{
    static f32 const inv255 = 1.0f / 255.0f;

    V4 v;
    v.a = (f32)a * inv255;

    // NB: Premultiplied alpha!
    v.r = v.a * ((f32)r * inv255);
    v.g = v.a * ((f32)g * inv255);
    v.b = v.a * ((f32)b * inv255);

    return v;
}

V4 makeWhite()
{
    return v4(1.0f, 1.0f, 1.0f, 1.0f);
}

V4 asOpaque(V4 color)
{
    // Colors are always stored with premultiplied alpha, so in order to set the alpha
    // back to 100%, we have to divide by that alpha.

    V4 result = color;

    // If alpha is 0, we can't divide, so just set it
    if (result.a == 0.0f) {
        result.a = 1.0f;
    }
    // If alpha is already 1, we don't need to do anything
    // If alpha is > 1, clamp it
    else if (result.a >= 1.0f) {
        result.a = 1.0f;
    }
    // Otherwise, divide by the alpha and then make it 1
    else {
        result.r = result.r / result.a;
        result.g = result.g / result.a;
        result.b = result.b / result.a;

        result.a = 1.0f;
    }

    return result;
}
