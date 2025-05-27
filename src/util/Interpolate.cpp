/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Interpolate.h"
#include "Maths.h"

f32 interpolate(f32 start, f32 end, f32 t, InterpolationType interpolation)
{
    f32 range = end - start;

    f32 interpolatedT = t;

    switch (interpolation) {
    case Interpolate_Linear: {
        interpolatedT = t;
    } break;

    case Interpolate_Sine: {
        interpolatedT = (1 - cos32(t * PI32)) * 0.5f;
    } break;
    case Interpolate_SineIn: {
        interpolatedT = 1 - cos32(t * PI32 * 0.5f);
    } break;
    case Interpolate_SineOut: {
        interpolatedT = sin32(t * PI32 * 0.5f);
    } break;

        INVALID_DEFAULT_CASE;
    }

    return start + (range * interpolatedT);
}
