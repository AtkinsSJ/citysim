/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>
#include <Util/Basic.h>
#include <initializer_list>

// Like an array, but indexed by an enum class.
// The enum must have a COUNT member that's the number of entries in the enum.
template<typename Enum, typename EntryType>
requires(__is_enum(Enum))
class EnumMap {
public:
    using UnderlyingType = __underlying_type(Enum);
    static constexpr u32 ENTRY_COUNT = to_underlying(Enum::COUNT);

    EnumMap() = default;

    explicit EnumMap(std::initializer_list<EntryType> initial_entries)
    {
        ASSERT(initial_entries.size() == ENTRY_COUNT);
        auto index = 0;
        for (auto& initial_entry : initial_entries) {
            m_values[index++] = move(initial_entry);
        }
    }

    EntryType& operator[](Enum e)
    {
        auto index = to_underlying(e);
        ASSERT(index >= 0 && index < ENTRY_COUNT);
        return m_values[index];
    }

    EntryType const& operator[](Enum e) const
    {
        return const_cast<EnumMap&>(*this)[e];
    }

    EntryType& operator[](UnderlyingType index)
    {
        ASSERT(index >= 0 && index < ENTRY_COUNT);
        return m_values[index];
    }

    EntryType const& operator[](UnderlyingType index) const
    {
        return const_cast<EnumMap&>(*this)[index];
    }

private:
    EntryType m_values[ENTRY_COUNT] {};
};
