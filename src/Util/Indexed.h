/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

template<typename T>
class Indexed {
public:
    Indexed(u32 index, T& value)
        : m_index(index)
        , m_value(value)
    {
    }

    // FIXME: Temporary, until we consistently use unsigned types for indexes
    Indexed(s32 index, T& value)
        : m_index(index)
        , m_value(value)
    {
    }

    u32 index() const { return m_index; }
    T& value() { return m_value; }
    T const& value() const { return m_value; }

    template<std::size_t Index>
    std::tuple_element_t<Index, Indexed>& get()
    {
        if constexpr (Index == 0)
            return m_index;
        if constexpr (Index == 1)
            return m_value;
        VERIFY_NOT_REACHED();
    }

private:
    u32 m_index;
    T& m_value;
};

namespace std {

template<typename T>
struct tuple_size<::Indexed<T>> {
    static constexpr size_t value = 2;
};

template<typename T>
struct tuple_element<0, ::Indexed<T>> {
    using type = u32;
};

template<typename T>
struct tuple_element<1, ::Indexed<T>> {
    using type = T&;
};

}
