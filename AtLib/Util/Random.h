/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Array.h>
#include <Util/Basic.h>
#include <Util/Forward.h>
#include <Util/Maths.h>
#include <Util/Optional.h>

class Random {
public:
    enum class Type : u8 {
        MT,
    };
    static OwnedRef<Random> create(Optional<u32> seed = {}, Optional<Type> = {});

    virtual ~Random() = default;

    virtual void reseed(u32 seed) = 0;

    // FIXME: This API could be a lot better. Figure out what we want once this is working.
    virtual u32 next() = 0;

    template<typename T>
    T random_integer()
    {
        if constexpr (sizeof(T) == sizeof(u64)) {
            u64 const high = next();
            u64 const low = next();
            return static_cast<T>(high << 32 | low);
        }
        if constexpr (sizeof(T) == sizeof(u32))
            return static_cast<T>(next());
        if constexpr (sizeof(T) == sizeof(u16))
            return static_cast<T>(next() & 0xFFFF);
        if constexpr (sizeof(T) == sizeof(u8))
            return static_cast<T>(next() & 0xFF);
        VERIFY_NOT_REACHED();
    }

    template<Integral T>
    T random_between(T min_inclusive, T max_exclusive)
    {
        // If the max is less than the min, just return the min.
        if (max_exclusive <= min_inclusive)
            return min_inclusive;

        T range = max_exclusive - min_inclusive;
        return min_inclusive + (next() % range);
    }

    template<Integral T>
    T random_below(T max_exclusive)
    {
        // 0 or negative max values don't make sense, so we return a 0 for those.
        if (max_exclusive <= 0)
            return 0;

        return static_cast<T>(next() % max_exclusive);
    }

    bool random_bool();
    float random_float_between(float min_inclusive, float max_exclusive);
    float random_float_0_1();

    void fill_with_noise(Array<float>& destination, s32 smoothing_pass_count, bool wrap = false);

protected:
    explicit Random() = default;
};
