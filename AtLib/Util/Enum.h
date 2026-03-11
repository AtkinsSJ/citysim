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
concept Enum = IsEnum<T>;
template<Enum T>
using EnumUnderlyingType = __underlying_type(T);

template<Enum T>
constexpr auto to_underlying(T enum_type)
{
    return static_cast<__underlying_type(T)>(enum_type);
}

template<typename T>
concept CountedEnum = IsEnum<T> && requires { T::COUNT; };

template<CountedEnum T>
inline constexpr u64 EnumCount = to_underlying(T::COUNT);

namespace Details {

template<CountedEnum>
struct EnumFlagStorageType {
    using Type = void;
};

template<CountedEnum T>
requires(EnumCount<T> < 8)
struct EnumFlagStorageType<T> {
    using Type = u8;
};

template<CountedEnum T>
requires(EnumCount<T> >= 8 && EnumCount<T> < 16)
struct EnumFlagStorageType<T> {
    using Type = u16;
};

template<CountedEnum T>
requires(EnumCount<T> >= 16 && EnumCount<T> < 32)
struct EnumFlagStorageType<T> {
    using Type = u32;
};

template<CountedEnum T>
requires(EnumCount<T> >= 32 && EnumCount<T> < 64)
struct EnumFlagStorageType<T> {
    using Type = u64;
};

}

template<CountedEnum T>
using EnumFlagStorageType = typename Details::EnumFlagStorageType<T>::Type;

template<CountedEnum T>
class EnumIterator {
public:
    explicit EnumIterator(T start)
        : m_value(start)
    {
    }

    static EnumIterator begin()
    {
        return EnumIterator { static_cast<T>(0) };
    }

    static EnumIterator end()
    {
        return EnumIterator { T::COUNT };
    }

    bool operator==(EnumIterator const&) const = default;
    bool operator==(T const& other) const { return m_value == other; }
    T operator*() const { return m_value; }
    EnumIterator& operator++()
    {
        m_value = static_cast<T>(to_underlying(m_value) + 1);
        return *this;
    }

private:
    T m_value;
};

template<CountedEnum T>
EnumIterator<T> enum_values()
{
    return EnumIterator<T>::begin();
}
