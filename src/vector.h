#pragma once

/**********************************************
    V2
 **********************************************/

struct V2 {
    f32 x;
    f32 y;
};

V2 v2(f32 x, f32 y);
V2 v2(s32 x, s32 y);
V2 v2(V2I source);

f32 lengthOf(V2 v);
f32 lengthSquaredOf(f32 x, f32 y);
V2 limit(V2 vector, f32 maxLength);
V2 lerp(V2 a, V2 b, f32 position);

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

inline V2 operator*(V2 v, f32 s)
{
    return v2(v.x * s, v.y * s);
}

inline V2 operator*=(V2& v, f32 s)
{
    v = v * s;
    return v;
}

inline V2 operator/(V2 v, f32 s)
{
    return v2(v.x / s, v.y / s);
}

inline V2 operator/=(V2& v, f32 s)
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
};

V2I v2i(s32 x, s32 y);
V2I v2i(V2 source);

f32 lengthOf(s32 x, s32 y);
f32 lengthOf(V2I v);

// Degrees!
f32 angleOf(s32 x, s32 y);

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
            f32 x;
            f32 y;
            f32 z;
        };
        struct
        {
            V2 xy;
        };
    };
};

V3 v3(f32 x, f32 y, f32 z);
V3 v3(s32 x, s32 y, s32 z);

f32 lengthOf(V3 v);

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

inline V3 operator*(V3 v, f32 s)
{
    return v3(v.x * s, v.y * s, v.z * s);
}

inline V3 operator*=(V3& v, f32 s)
{
    v = v * s;
    return v;
}

inline V3 operator/(V3 v, f32 s)
{
    return v3(v.x / s, v.y / s, v.z / s);
}

inline V3 operator/=(V3& v, f32 s)
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
            f32 x;
            f32 y;
            f32 z;
            f32 w;
        };
        struct
        {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
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

V4 v4(f32 x, f32 y, f32 z, f32 w);
V4 v4(s32 x, s32 y, s32 z, s32 w);

f32 lengthOf(V4 v);

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

inline V4 operator*(V4 v, f32 s)
{
    return v4(v.x * s, v.y * s, v.z * s, v.w * s);
}

inline V4 operator*=(V4& v, f32 s)
{
    v = v * s;
    return v;
}

inline V4 operator/(V4 v, f32 s)
{
    return v4(v.x / s, v.y / s, v.z / s, v.w / s);
}

inline V4 operator/=(V4& v, f32 s)
{
    v = v / s;
    return v;
}
