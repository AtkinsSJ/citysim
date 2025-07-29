/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Interpolate.h"
#include <Util/Maths.h>

float interpolate(float start, float end, float t, Interpolation interpolation)
{
    float range = end - start;

    float interpolatedT = t;

    switch (interpolation) {
    case Interpolation::Linear: {
        interpolatedT = t;
    } break;

    case Interpolation::Sine: {
        interpolatedT = (1 - cos32(t * PI32)) * 0.5f;
    } break;
    case Interpolation::SineIn: {
        interpolatedT = 1 - cos32(t * PI32 * 0.5f);
    } break;
    case Interpolation::SineOut: {
        interpolatedT = sin32(t * PI32 * 0.5f);
    } break;

        INVALID_DEFAULT_CASE;
    }

    return start + (range * interpolatedT);
}
