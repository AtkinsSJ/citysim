/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Basic.h"
#include "Forward.h"

float const PI32 = 3.14159265358979323846f;
float const radToDeg = 180.0f / PI32;
float const degToRad = PI32 / 180.0f;

// Standard rounding functions return doubles, so here's some int ones.
s32 round_s32(float in);
s32 floor_s32(float in);
s32 ceil_s32(float in);
s32 abs_s32(s32 in);
float round_float(float in);
float floor_float(float in);
float ceil_float(float in);
float sqrt_float(float in);
float abs_float(float in);
float fraction_float(float in);
float clamp01(float in);
float sin32(float radians);
float cos32(float radians);
float tan32(float radians);

s32 divideCeil(s32 numerator, s32 denominator);

s32 truncate32(s64 in);

u8 clamp01AndMap_u8(float in);

template<typename T>
T clamp(T value, T min, T max)
{
    ASSERT(min <= max); // min > max in clamp()!
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

template<typename T>
T min(T a, T b)
{
    return (a < b) ? a : b;
}

template<typename T>
inline T min(T a) { return a; }

template<typename T, typename... Args>
T min(T a, Args... args)
{
    T b = min(args...);
    return (a < b) ? a : b;
}

template<typename T>
T max(T a, T b)
{
    return (a > b) ? a : b;
}

template<typename T>
inline T max(T a) { return a; }

template<typename T, typename... Args>
T max(T a, Args... args)
{
    T b = max(args...);
    return (a > b) ? a : b;
}

template<typename T>
T wrap(T value, T max)
{
    return (value + max) % max;
}

template<typename T>
T lerp(T a, T b, float position)
{
    return (T)(a + (b - a) * position);
}

template<typename T>
T approach(T currentValue, T targetValue, T distance)
{
    T result = currentValue;

    if (targetValue < currentValue) {
        result = max(currentValue - distance, targetValue);
    } else {
        result = min(currentValue + distance, targetValue);
    }

    return result;
}

// How far is the point from the rectangle? Returns 0 if the point is inside the rectangle.
s32 manhattanDistance(Rect2I rect, V2I point);
s32 manhattanDistance(Rect2I a, Rect2I b);

bool equals(float a, float b, float epsilon);

//
// All this mess is just so we can access a type's min/max values from a template.
//
template<typename T>
inline T const minPossibleValue();
template<typename T>
inline T const maxPossibleValue();

template<>
inline u8 const minPossibleValue<u8>() { return 0; }
template<>
inline u8 const maxPossibleValue<u8>() { return u8Max; }
template<>
inline u16 const minPossibleValue<u16>() { return 0; }
template<>
inline u16 const maxPossibleValue<u16>() { return u16Max; }
template<>
inline u32 const minPossibleValue<u32>() { return 0; }
template<>
inline u32 const maxPossibleValue<u32>() { return u32Max; }
template<>
inline u64 const minPossibleValue<u64>() { return 0; }
template<>
inline u64 const maxPossibleValue<u64>() { return u64Max; }

template<>
inline s8 const minPossibleValue<s8>() { return s8Min; }
template<>
inline s8 const maxPossibleValue<s8>() { return s8Max; }
template<>
inline s16 const minPossibleValue<s16>() { return s16Min; }
template<>
inline s16 const maxPossibleValue<s16>() { return s16Max; }
template<>
inline s32 const minPossibleValue<s32>() { return s32Min; }
template<>
inline s32 const maxPossibleValue<s32>() { return s32Max; }
template<>
inline s64 const minPossibleValue<s64>() { return s64Min; }
template<>
inline s64 const maxPossibleValue<s64>() { return s64Max; }

template<>
inline float const minPossibleValue<float>() { return floatMin; }
template<>
inline float const maxPossibleValue<float>() { return floatMax; }
template<>
inline double const minPossibleValue<double>() { return f64Min; }
template<>
inline double const maxPossibleValue<double>() { return f64Max; }

template<typename T>
bool canCastIntTo(s64 input)
{
    return (input >= minPossibleValue<T>() && input <= maxPossibleValue<T>());
}

template<typename T>
T truncate(s64 in)
{
    ASSERT(canCastIntTo<T>(in));
    return (T)in;
}
