/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Random.h"

#include <Util/Maths.h>
#include <Util/Rectangle.h>
#include <ctime>

class MersenneTwister final : public Random {
public:
    explicit MersenneTwister(u32 seed)
    {
        reseed(seed);
    }
    virtual ~MersenneTwister() override = default;

    virtual void reseed(u32 seed) override
    {
        m_index = 624;
        m_mt[0] = seed;
        for (int i = 1; i < 624; i++) {
            m_mt[i] = 1812433253 * (m_mt[i - 1] ^ m_mt[i - 1] >> 30) + i;
        }
    }

    virtual u32 next() override
    {
        if (m_index >= 624)
            twist();

        u32 y = m_mt[m_index];

        y = y ^ (y >> 11);
        y = y ^ ((y << 7) & 2636928640);
        y = y ^ ((y << 15) & 4022730752);
        y = y ^ (y >> 18);

        m_index++;

        return y;
    }

private:
    void twist()
    {
        for (int i = 0; i < 624; i++) {
            u32 y = (m_mt[i] & 0x80000000) + (m_mt[(i + 1) % 624] & 0x7fffffff);

            m_mt[i] = m_mt[(i + 397) % 624] ^ y >> 1;

            if (y % 2) {
                m_mt[i] = m_mt[i] ^ 0x9908b0df;
            }
        }

        m_index = 0;
    }

    u32 m_mt[624];
    u32 m_index;
};

Random* Random::create(Optional<u32> seed, Optional<Type> type)
{
    auto actual_seed = [&seed] -> u32 {
        if (seed.has_value())
            return seed.value();
        return static_cast<u32>(time(nullptr));
    }();
    switch (type.value_or(Type::MT)) {
    case Type::MT:
        return new MersenneTwister(actual_seed);
    }
    VERIFY_NOT_REACHED();
}

s32 Random::random_below(s32 max_exclusive)
{
    // 0 or negative max values don't make sense, so we return a 0 for those.
    if (max_exclusive <= 0)
        return 0;

    return next() % max_exclusive;
}

s32 Random::random_between(s32 min_inclusive, s32 max_exclusive)
{
    // If the max is less than the min, just return the min.
    if (max_exclusive <= min_inclusive)
        return min_inclusive;

    s32 range = max_exclusive - min_inclusive;
    return min_inclusive + (next() % range);
}

bool Random::random_bool()
{
    return (next() % 2) != 0;
}

float Random::random_float_between(float min_inclusive, float max_exclusive)
{
    return (random_float_0_1() * (max_exclusive - min_inclusive)) + min_inclusive;
}

float Random::random_float_0_1()
{
    return abs_float(static_cast<float>(next()) / static_cast<float>(s32Max));
}

void Random::fill_with_noise(Array<float>& destination, s32 smoothing_pass_count, bool wrap)
{
    for (s32 i = 0; i < destination.count; i++) {
        destination[i] = random_float_0_1();
    }

    // Smoothing
    for (s32 iteration = 0; iteration < smoothing_pass_count; iteration++) {
        if (wrap) {
            // Front
            destination[0] = (destination[destination.count - 1] + destination[0] + destination[1]) / 3.0f;
            // Back
            destination[destination.count - 1] = (destination[destination.count - 2] + destination[destination.count - 1] + destination[0]) / 3.0f;
        } else {
            // Fake normalisation because otherwise the ends get weird

            // Front
            destination[0] = (destination[0] + destination[1]) / 2.0f;
            // Back
            destination[destination.count - 1] = (destination[destination.count - 2] + destination[destination.count - 1]) / 2.0f;
        }

        for (s32 i = 1; i < destination.count - 1; i++) {
            destination[i] = (destination[i - 1] + destination[i] + destination[i + 1]) / 3.0f;
        }
    }
}
