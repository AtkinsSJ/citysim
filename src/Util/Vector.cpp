/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Vector.h"
#include "Maths.h"
#include <cmath>

/**********************************************
    V2
 **********************************************/

V2 v2(f32 x, f32 y)
{
    V2 result;
    result.x = x;
    result.y = y;
    return result;
}

V2 v2(s32 x, s32 y)
{
    return v2((f32)x, (f32)y);
}

V2 v2(V2I source)
{
    return v2(source.x, source.y);
}

f32 lengthOf(V2 v)
{
    return (f32)sqrt(v.x * v.x + v.y * v.y);
}

f32 lengthSquaredOf(f32 x, f32 y)
{
    return (f32)(x * x + y * y);
}

V2 limit(V2 vector, f32 maxLength)
{
    f32 length = lengthOf(vector);
    if (length > maxLength) {
        vector *= maxLength / length;
    }
    return vector;
}

V2 lerp(V2 a, V2 b, f32 position)
{
    return a + (b - a) * position;
}

/**********************************************
    V2I
 **********************************************/

V2I v2i(s32 x, s32 y)
{
    V2I result;
    result.x = x;
    result.y = y;
    return result;
}

V2I v2i(V2 source)
{
    return v2i(floor_s32(source.x), floor_s32(source.y));
}

f32 lengthOf(V2I v)
{
    return (f32)sqrt(v.x * v.x + v.y * v.y);
}

f32 lengthOf(s32 x, s32 y)
{
    return (f32)sqrt(x * x + y * y);
}

f32 angleOf(s32 x, s32 y)
{
    return (f32)fmod((atan2(y, x) * radToDeg) + 360.0f, 360.0f);
}

/**********************************************
    V3
 **********************************************/

V3 v3(f32 x, f32 y, f32 z)
{
    V3 v;
    v.x = x;
    v.y = y;
    v.z = z;

    return v;
}

V3 v3(s32 x, s32 y, s32 z)
{
    return v3((f32)x, (f32)y, (f32)z);
}

f32 lengthOf(V3 v)
{
    return (f32)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

/**********************************************
    V4
 **********************************************/

V4 v4(f32 x, f32 y, f32 z, f32 w)
{
    V4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;

    return v;
}

V4 v4(s32 x, s32 y, s32 z, s32 w)
{
    return v4((f32)x, (f32)y, (f32)z, (f32)w);
}

f32 lengthOf(V4 v)
{
    return (f32)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}
