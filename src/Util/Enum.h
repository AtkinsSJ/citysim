/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

template<typename T>
inline constexpr bool IsEnum = __is_enum(T);

template<typename T>
concept Enum = IsEnum<T> && requires { T::COUNT; };

template<Enum T>
using EnumUnderlyingType = __underlying_type(T);

template<Enum T>
inline constexpr u64 EnumCount = to_underlying(T::COUNT);

namespace Details {

template<Enum>
struct EnumFlagStorageType {
    using Type = void;
};

template<Enum T>
requires(EnumCount<T> < 8)
struct EnumFlagStorageType<T> {
    using Type = u8;
};

template<Enum T>
requires(EnumCount<T> >= 8 && EnumCount<T> < 16)
struct EnumFlagStorageType<T> {
    using Type = u16;
};

template<Enum T>
requires(EnumCount<T> >= 16 && EnumCount<T> < 32)
struct EnumFlagStorageType<T> {
    using Type = u32;
};

template<Enum T>
requires(EnumCount<T> >= 32 && EnumCount<T> < 64)
struct EnumFlagStorageType<T> {
    using Type = u64;
};

}

template<Enum T>
using EnumFlagStorageType = typename Details::EnumFlagStorageType<T>::Type;
