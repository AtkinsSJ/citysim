/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Forward.h>

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
constexpr T clamp(T value, T min, T max)
{
    ASSERT(min <= max); // min > max in clamp()!
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

template<typename T>
constexpr T min(T a, T b)
{
    return (a < b) ? a : b;
}

template<typename T>
constexpr T min(T a) { return a; }

template<typename T, typename... Args>
constexpr T min(T a, Args... args)
{
    T b = min(args...);
    return (a < b) ? a : b;
}

template<typename T>
constexpr T max(T a, T b)
{
    return (a > b) ? a : b;
}

template<typename T>
constexpr T max(T a) { return a; }

template<typename T, typename... Args>
constexpr T max(T a, Args... args)
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

bool equals_with_epsilon(float a, float b, float epsilon);

template<typename T>
inline constexpr T MaxValue;
template<typename T>
inline constexpr T MinValue;
template<>
inline constexpr u8 MinValue<u8> = u8Min;
template<>
inline constexpr u8 MaxValue<u8> = u8Max;
template<>
inline constexpr u16 MinValue<u16> = u16Min;
template<>
inline constexpr u16 MaxValue<u16> = u16Max;
template<>
inline constexpr u32 MinValue<u32> = u32Min;
template<>
inline constexpr u32 MaxValue<u32> = u32Max;
template<>
inline constexpr u64 MinValue<u64> = u64Min;
template<>
inline constexpr u64 MaxValue<u64> = u64Max;
template<>
inline constexpr s8 MinValue<s8> = s8Min;
template<>
inline constexpr s8 MaxValue<s8> = s8Max;
template<>
inline constexpr s16 MinValue<s16> = s16Min;
template<>
inline constexpr s16 MaxValue<s16> = s16Max;
template<>
inline constexpr s32 MinValue<s32> = s32Min;
template<>
inline constexpr s32 MaxValue<s32> = s32Max;
template<>
inline constexpr s64 MinValue<s64> = s64Min;
template<>
inline constexpr s64 MaxValue<s64> = s64Max;

template<>
inline constexpr float MinValue<float> = floatMin;
template<>
inline constexpr float MaxValue<float> = floatMax;
template<>
inline constexpr double MinValue<double> = f64Min;
template<>
inline constexpr double MaxValue<double> = f64Max;

template<typename T>
constexpr bool is_within_integer_range(s64 input)
{
    return input >= MinValue<T> && input <= MaxValue<T>;
}

template<typename T>
T truncate(s64 in)
{
    ASSERT(is_within_integer_range<T>(in));
    return (T)in;
}
