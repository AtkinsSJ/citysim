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

V2 v2(float x, float y)
{
    V2 result;
    result.x = x;
    result.y = y;
    return result;
}

V2 v2(s32 x, s32 y)
{
    return v2((float)x, (float)y);
}

V2 v2(V2I source)
{
    return v2(source.x, source.y);
}

float lengthOf(V2 v)
{
    return (float)sqrt(v.x * v.x + v.y * v.y);
}

float lengthSquaredOf(float x, float y)
{
    return (float)(x * x + y * y);
}

V2 limit(V2 vector, float maxLength)
{
    float length = lengthOf(vector);
    if (length > maxLength) {
        vector *= maxLength / length;
    }
    return vector;
}

V2 lerp(V2 a, V2 b, float position)
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

float lengthOf(V2I v)
{
    return (float)sqrt(v.x * v.x + v.y * v.y);
}

float lengthOf(s32 x, s32 y)
{
    return (float)sqrt(x * x + y * y);
}

float angleOf(s32 x, s32 y)
{
    return (float)fmod((atan2(y, x) * radToDeg) + 360.0f, 360.0f);
}

/**********************************************
    V3
 **********************************************/

V3 v3(float x, float y, float z)
{
    V3 v;
    v.x = x;
    v.y = y;
    v.z = z;

    return v;
}

V3 v3(s32 x, s32 y, s32 z)
{
    return v3((float)x, (float)y, (float)z);
}

float lengthOf(V3 v)
{
    return (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

/**********************************************
    V4
 **********************************************/

V4 v4(float x, float y, float z, float w)
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
    return v4((float)x, (float)y, (float)z, (float)w);
}

float lengthOf(V4 v)
{
    return (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}
