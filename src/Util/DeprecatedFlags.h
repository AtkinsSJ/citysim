/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

//
// I've repeatedly found I want an enum that can be used as a set of flags, but also be usable as indices into an array.
// I've had to define the enums twice in this case, which is awkward. So, this is a flags field that takes a 0,1,2,3...N
// enum and turns it into a bitfield internally, using (1 << flag) to convert to a bit position.
// It's accessed the same way as a normal bitfield would be. & to check a flag, |= to add one and ^= to remove one.
// You can also use isEmpty() to check if anything is set, which is useful sometimes.
//
// A quirk of how it works is, you can't pass multiple flags at the same time. Maybe I'll change that later.
// But for now, only pure enum values will work, so we couldn't be able to muddle-up different enums by mistake.
//
// It's definitely not a replacement for ALL flag fields. But it's clearer when you see...
//     Flags<BuildingFlag> flags;
// ...than when you see...
//     u32 flags;
// ...and that should mean fewer chances for bugs.
//
// - Sam, 01/09/2019
//

template<typename Enum, typename Storage>
struct DeprecatedFlags {
    s32 flagCount;
    Storage data;

    // Read the value of the flag
    bool operator&(Enum flag)
    {
        bool result = false;

        if (flag < 0 || flag >= this->flagCount) {
            ASSERT(false);
        } else {
            result = (this->data & ((u64)1 << flag)) != 0;
        }

        return result;
    }

    // Add the flag
    DeprecatedFlags<Enum, Storage>* operator+=(Enum flag)
    {
        this->data |= ((u64)1 << flag);
        return this;
    }

    // Remove the flag
    DeprecatedFlags<Enum, Storage>* operator-=(Enum flag)
    {
        u64 mask = ((u64)1 << flag);
        this->data &= ~mask;
        return this;
    }

    // Toggle the flag
    DeprecatedFlags<Enum, Storage>* operator^=(Enum flag)
    {
        this->data ^= ((u64)1 << flag);
        return this;
    }

    // Comparison
    bool operator==(DeprecatedFlags<Enum, Storage>& other)
    {
        return this->data == other.data;
    }

    // Comparison
    bool operator!=(DeprecatedFlags<Enum, Storage>& other)
    {
        return !(*this == other);
    }
};

template<typename Enum>
using Flags8 = DeprecatedFlags<Enum, u8>;
template<typename Enum>
using Flags16 = DeprecatedFlags<Enum, u16>;
template<typename Enum>
using Flags32 = DeprecatedFlags<Enum, u32>;
template<typename Enum>
using Flags64 = DeprecatedFlags<Enum, u64>;

template<typename Enum, typename Storage>
void initFlags(DeprecatedFlags<Enum, Storage>* flags, Enum flagCount)
{
    ASSERT(flagCount <= (8 * sizeof(Storage)));

    flags->flagCount = flagCount;
    flags->data = 0;
}

template<typename Enum, typename Storage>
bool isEmpty(DeprecatedFlags<Enum, Storage>* flags)
{
    return (flags->data == 0);
}

template<typename Enum, typename Storage>
Storage getAll(DeprecatedFlags<Enum, Storage>* flags)
{
    return flags->data;
}
