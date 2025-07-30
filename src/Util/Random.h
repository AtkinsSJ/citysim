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
    static Random* create(Optional<u32> seed = {}, Optional<Type> = {});

    virtual ~Random() = default;

    virtual void reseed(u32 seed) = 0;

    // FIXME: This API could be a lot better. Figure out what we want once this is working.
    virtual u32 next() = 0;

    template<typename T>
    T random_integer()
    {
        static_assert(sizeof(T) < 8);
        return static_cast<T>(random_between(minPossibleValue<T>(), maxPossibleValue<T>() + 1));
    }

    s32 random_below(s32 max_exclusive);
    s32 random_between(s32 min_inclusive, s32 max_exclusive);

    bool random_bool();
    float random_float_between(float min_inclusive, float max_exclusive);
    float random_float_0_1();

    void fill_with_noise(Array<float>& destination, s32 smoothing_pass_count, bool wrap = false);

protected:
    explicit Random() = default;
};
