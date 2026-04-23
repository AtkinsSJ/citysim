/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Concepts.h>
#include <typeindex>

using Hash = u32;

// FNV-1a hash
// http://www.isthe.com/chongo/tech/comp/fnv/
inline Hash hash_u32(u32 value)
{
    return (value ^ 2166136261) * 16777619;
}

inline Hash pair_hash(Hash a, Hash b)
{
    return (a ^ b) * 16777619;
}

inline Hash hash_u64(u64 value)
{
    u32 low = value & 0xffff'ffff;
    u32 high = (value & 0xffff'ffff'0000'0000) >> 32;
    return pair_hash(hash_u32(low), hash_u32(high));
}

template<typename T>
concept HashableObject = requires(T const& a, T const& b) {
    a.hash();
    a.equals(b);
};

template<typename T>
struct DefaultHashTraits {
    static bool equals(T const& a, T const& b) { return a == b; }
};

template<typename T>
struct HashTraits : DefaultHashTraits<T> { };

template<Integral T>
struct HashTraits<T> : DefaultHashTraits<T> {
    static Hash hash(T const& t)
    {
        if constexpr (sizeof(T) == 8)
            return hash_u64(t);
        else
            return hash_u32(t);
    }
};

template<typename T>
struct HashTraits<T*> {
    static bool equals(T const* a, T const* b)
    {
        return a == b;
    }

    static Hash hash(T const* t)
    {
        return HashTraits<smm>::hash(reinterpret_cast<smm>(t));
    }
};

template<HashableObject T>
struct HashTraits<T> : DefaultHashTraits<T> {
    static bool equals(T const& a, T const& b)
    {
        return a.equals(b);
    }

    static Hash hash(T const& t)
    {
        return t.hash();
    }
};

// FIXME: We can probably implement this for anything that has std::hash defined.
template<>
struct HashTraits<std::type_index> {
    static bool equals(std::type_index const& a, std::type_index const& b)
    {
        return a == b;
    }

    static Hash hash(std::type_index const& t)
    {
        return hash_u64(t.hash_code());
    }
};

template<typename T>
concept Hashable = requires(T const& t) {
    HashTraits<T>::hash(t);
};
