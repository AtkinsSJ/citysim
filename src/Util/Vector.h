/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Util/Basic.h>
#include <Util/Forward.h>

/**********************************************
    V2
 **********************************************/

struct V2 {
    float x;
    float y;

    V2 operator-() const { return { -x, -y }; }
};

V2 v2(float x, float y);
V2 v2(s32 x, s32 y);
V2 v2(V2I source);

float lengthOf(V2 v);
float lengthSquaredOf(float x, float y);
V2 limit(V2 vector, float maxLength);
V2 lerp(V2 a, V2 b, float position);

inline V2 operator+(V2 lhs, V2 rhs)
{
    return v2(lhs.x + rhs.x, lhs.y + rhs.y);
}

inline V2 operator+=(V2& lhs, V2 rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

inline V2 operator-(V2 lhs, V2 rhs)
{
    return v2(lhs.x - rhs.x, lhs.y - rhs.y);
}

inline V2 operator-=(V2& lhs, V2 rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

inline V2 operator*(V2 v, float s)
{
    return v2(v.x * s, v.y * s);
}

inline V2 operator*=(V2& v, float s)
{
    v = v * s;
    return v;
}

inline V2 operator/(V2 v, float s)
{
    return v2(v.x / s, v.y / s);
}

inline V2 operator/=(V2& v, float s)
{
    v = v / s;
    return v;
}

/**********************************************
    V2I
 **********************************************/

struct V2I {
    s32 x;
    s32 y;

    static Optional<V2I> read(LineReader&);

    bool operator==(V2I const&) const = default;
};

V2I v2i(s32 x, s32 y);
V2I v2i(V2 source);

float lengthOf(s32 x, s32 y);
float lengthOf(V2I v);

// Degrees!
float angleOf(s32 x, s32 y);

inline V2I operator+(V2I lhs, V2I rhs)
{
    return v2i(lhs.x + rhs.x, lhs.y + rhs.y);
}

inline V2I operator+=(V2I& lhs, V2I rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

inline V2I operator-(V2I lhs, V2I rhs)
{
    return v2i(lhs.x - rhs.x, lhs.y - rhs.y);
}

inline V2I operator-=(V2I& lhs, V2I rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

inline V2I operator*(V2I v, s32 s)
{
    return v2i(v.x * s, v.y * s);
}

inline V2I operator*=(V2I& v, s32 s)
{
    v = v * s;
    return v;
}

inline V2I operator/(V2I v, s32 s)
{
    return v2i(v.x / s, v.y / s);
}

inline V2I operator/=(V2I& v, s32 s)
{
    v = v / s;
    return v;
}

/**********************************************
    V3
 **********************************************/

struct V3 {
    union {
        struct
        {
            float x;
            float y;
            float z;
        };
        struct
        {
            V2 xy;
        };
    };
};

V3 v3(float x, float y, float z);
V3 v3(s32 x, s32 y, s32 z);

float lengthOf(V3 v);

inline V3 operator+(V3 lhs, V3 rhs)
{
    return v3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}

inline V3 operator+=(V3& lhs, V3 rhs)
{
    lhs = lhs + rhs;
    return lhs;
}

inline V3 operator-(V3 lhs, V3 rhs)
{
    return v3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}

inline V3 operator-=(V3& lhs, V3 rhs)
{
    lhs = lhs - rhs;
    return lhs;
}

inline V3 operator*(V3 v, float s)
{
    return v3(v.x * s, v.y * s, v.z * s);
}

inline V3 operator*=(V3& v, float s)
{
    v = v * s;
    return v;
}

inline V3 operator/(V3 v, float s)
{
    return v3(v.x / s, v.y / s, v.z / s);
}

inline V3 operator/=(V3& v, float s)
{
    v = v / s;
    return v;
}

/**********************************************
    V4
 **********************************************/

struct V4 {
    union {
        struct
        {
            float x;
            float y;
            float z;
            float w;
        };
        struct
        {
            float r;
            float g;
            float b;
            float a;
        };
        struct
        {
            V3 xyz;
        };
        struct
        {
            V2 xy;
        };
    };
};

V4 v4(float x, float y, float z, float w);
V4 v4(s32 x, s32 y, s32 z, s32 w);

float lengthOf(V4 v);

inline V4 operator+(V4 lhs, V4 rhs)
{
    return v4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
}

inline V4 operator+=(V4& lhs, V4 rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    lhs.w += rhs.w;

    return lhs;
}

inline V4 operator-(V4 lhs, V4 rhs)
{
    return v4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
}

inline V4 operator-=(V4& lhs, V4 rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    lhs.w -= rhs.w;

    return lhs;
}

inline V4 operator*(V4 lhs, V4 rhs)
{
    return v4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
}

inline V4 operator*=(V4& lhs, V4 rhs)
{
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    lhs.z *= rhs.z;
    lhs.w *= rhs.w;

    return lhs;
}

inline V4 operator*(V4 v, float s)
{
    return v4(v.x * s, v.y * s, v.z * s, v.w * s);
}

inline V4 operator*=(V4& v, float s)
{
    v = v * s;
    return v;
}

inline V4 operator/(V4 v, float s)
{
    return v4(v.x / s, v.y / s, v.z / s, v.w / s);
}

inline V4 operator/=(V4& v, float s)
{
    v = v / s;
    return v;
}
