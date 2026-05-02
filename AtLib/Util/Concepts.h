/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

template<typename T, typename U>
inline constexpr bool IsSame = false;

template<typename T>
inline constexpr bool IsSame<T, T> = true;

template<typename T, typename U>
concept SameAs = IsSame<T, U>;

template<typename To, typename From>
inline constexpr bool IsConvertibleTo = requires { declval<void (*)(To)>()(declval<From>()); };

template<typename To, typename From>
concept ConvertibleTo = IsConvertibleTo<To, From>;

template<typename Callable, typename Out, typename... Arguments>
inline constexpr bool IsCallableWithArguments = requires(Callable callable) {
    {
        callable(declval<Arguments>()...)
    } -> ConvertibleTo<Out>;
} || requires(Callable callable) {
    {
        callable(declval<Arguments>()...)
    } -> SameAs<Out>;
};

template<typename T>
inline constexpr bool IsIntegral = false;

template<>
inline constexpr bool IsIntegral<u8> = true;
template<>
inline constexpr bool IsIntegral<u16> = true;
template<>
inline constexpr bool IsIntegral<u32> = true;
template<>
inline constexpr bool IsIntegral<u64> = true;
template<>
inline constexpr bool IsIntegral<s8> = true;
template<>
inline constexpr bool IsIntegral<s16> = true;
template<>
inline constexpr bool IsIntegral<s32> = true;
template<>
inline constexpr bool IsIntegral<s64> = true;

template<typename T>
concept Integral = IsIntegral<T>;
