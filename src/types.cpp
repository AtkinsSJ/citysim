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
