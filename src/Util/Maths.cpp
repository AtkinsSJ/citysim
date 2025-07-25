/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */



#include "Maths.h"

#include "Rectangle.h"

#include <cmath>

// Standard rounding functions return doubles, so here's some int ones.
s32 round_s32(float in)
{
    return (s32)round(in);
}

s32 floor_s32(float in)
{
    return (s32)floor(in);
}

s32 ceil_s32(float in)
{
    return (s32)ceil(in);
}

s32 abs_s32(s32 in)
{
    return (in < 0) ? -in : in;
}

float round_float(float in)
{
    return (float)round(in);
}

float floor_float(float in)
{
    return (float)floor(in);
}

float ceil_float(float in)
{
    return (float)ceil(in);
}

float sqrt_float(float in)
{
    return (float)sqrt(in);
}

float abs_float(float in)
{
    return (in < 0.0f) ? -in : in;
}

float fraction_float(float in)
{
    return (float)fmod(in, 1.0f);
}

float sin32(float radians)
{
    return (float)sin(radians);
}

float cos32(float radians)
{
    return (float)cos(radians);
}

float tan32(float radians)
{
    return (float)tan(radians);
}

float atan2_float(float y, float x)
{
    return atan2(y, x);
}

s32 divideCeil(s32 numerator, s32 denominator)
{
    return (numerator + denominator - 1) / denominator;
}

s32 truncate32(s64 in)
{
    ASSERT(in <= s32Max); // Value is too large to truncate to s32!
    return (s32)in;
}

float clamp01(float in)
{
    return clamp(in, 0.0f, 1.0f);
}

u8 clamp01AndMap_u8(float in)
{
    return (u8)(clamp01(in) * 255.0f);
}

// How far is the point from the rectangle? Returns 0 if the point is inside the rectangle.
s32 manhattanDistance(Rect2I rect, V2I point)
{
    s32 result = 0;

    if (point.x < rect.x) {
        result += rect.x - point.x;
    } else if (point.x >= (rect.x + rect.w)) {
        result += 1 + point.x - (rect.x + rect.w);
    }

    if (point.y < rect.y) {
        result += rect.y - point.y;
    } else if (point.y >= (rect.y + rect.h)) {
        result += 1 + point.y - (rect.y + rect.h);
    }

    return result;
}

s32 manhattanDistance(Rect2I a, Rect2I b)
{
    s32 result = 0;

    if (a.x + a.w <= b.x) {
        result += 1 + b.x - (a.x + a.w);
    } else if (b.x + b.w <= a.x) {
        result += 1 + a.x - (b.x + b.w);
    }

    if (a.y + a.h <= b.y) {
        result += 1 + b.y - (a.y + a.h);
    } else if (b.y + b.h <= a.y) {
        result += 1 + a.y - (b.y + b.h);
    }

    return result;
}

bool equals(float a, float b, float epsilon)
{
    return (abs_float(a - b) < epsilon);
}
