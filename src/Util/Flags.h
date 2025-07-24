/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Enum.h>

template<Enum EnumT>
class Flags {
public:
    using StorageT = EnumFlagStorageType<EnumT>;

    Flags() = default;
    constexpr Flags(std::initializer_list<EnumT> initial_flags)
    {
        for (auto flag : initial_flags)
            m_data |= 1u << to_underlying(flag);
    }

    bool flag_is_valid(EnumT flag) const
    {
        return to_underlying(flag) >= 0 && to_underlying(flag) < EnumCount<EnumT>;
    }

    bool has(EnumT flag) const
    {
        ASSERT(flag_is_valid(flag));
        return (m_data & (1u << to_underlying(flag))) != 0;
    }

    bool has_all(Flags const& other) const
    {
        return (m_data & other.m_data) == other.m_data;
    }

    bool has_any(Flags const& other) const
    {
        return (m_data & other.m_data) != 0;
    }

    Flags& add(EnumT flag)
    {
        ASSERT(flag_is_valid(flag));
        m_data |= 1u << to_underlying(flag);
        return *this;
    }

    Flags& add_all(Flags other)
    {
        m_data |= other.m_data;
        return *this;
    }

    Flags& remove(EnumT flag)
    {
        ASSERT(flag_is_valid(flag));
        m_data &= ~(1u << to_underlying(flag));
        return *this;
    }

    Flags& remove_all(Flags other)
    {
        m_data &= ~other.m_data;
        return *this;
    }

    Flags& toggle(EnumT flag)
    {
        ASSERT(flag_is_valid(flag));
        m_data ^= 1u << to_underlying(flag);
        return *this;
    }

    bool is_empty() const
    {
        return m_data == 0;
    }

    operator StorageT() const
    {
        return m_data;
    }

    bool operator==(Flags const& other) const
    {
        return m_data == other.m_data;
    }

    bool operator!=(Flags const& other) const
    {
        return !(*this == other);
    }

private:
    StorageT m_data { 0 };
};
