/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <cfloat>
#include <cstdint>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

s8 constexpr s8Min = INT8_MIN;
s8 constexpr s8Max = INT8_MAX;
s16 constexpr s16Min = INT16_MIN;
s16 constexpr s16Max = INT16_MAX;
s32 constexpr s32Min = INT32_MIN;
s32 constexpr s32Max = INT32_MAX;
s64 constexpr s64Min = INT64_MIN;
s64 constexpr s64Max = INT64_MAX;

u8 constexpr u8Max = UINT8_MAX;
u16 constexpr u16Max = UINT16_MAX;
u32 constexpr u32Max = UINT32_MAX;
u64 constexpr u64Max = UINT64_MAX;

f32 constexpr f32Min = -FLT_MAX;
f32 constexpr f32Max = FLT_MAX;
f64 constexpr f64Min = -DBL_MAX;
f64 constexpr f64Max = DBL_MAX;

typedef intptr_t smm;
// typedef uintptr_t umm; // Turned this off because I don't think there's a good reason for using it?

#include <uchar.h>
typedef char32_t unichar;

template<typename T>
requires(__is_enum(T))
constexpr auto to_underlying(T enum_type)
{
    return static_cast<__underlying_type(T)>(enum_type);
}
