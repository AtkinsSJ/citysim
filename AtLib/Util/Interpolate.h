/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

enum class Interpolation : u8 {
    Linear,

    Sine,
    SineIn,
    SineOut,
};

// t is a value from 0 to 1, representing how far through the interpolation you are
float interpolate(float start, float end, float t, Interpolation interpolation);
