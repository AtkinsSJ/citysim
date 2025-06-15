/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Basic.h"

enum InterpolationType {
    Interpolate_Linear,

    Interpolate_Sine,
    Interpolate_SineIn,
    Interpolate_SineOut,

    InterpolationTypeCount
};

// t is a value from 0 to 1, representing how far through the interpolation you are
f32 interpolate(f32 start, f32 end, f32 t, InterpolationType interpolation);
