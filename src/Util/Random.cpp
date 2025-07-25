/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Random.h"

#include "../types.h"
#include "Maths.h"
#include "Rectangle.h"

void initRandom(Random* random, RandomType type, s32 seed)
{
    random->type = type;

    switch (type) {
    case RandomType::MT: {
        MT_randomSeed(&random->mt, seed);
    } break;

        INVALID_DEFAULT_CASE;
    }
}

s32 randomNext(Random* random)
{
    s32 result = 0;

    switch (random->type) {
    case RandomType::MT: {
        result = MT_randomNext(&random->mt);
    } break;

        INVALID_DEFAULT_CASE;
    }

    return result;
}

s32 randomBelow(Random* random, s32 maxExclusive)
{
    s32 result = 0;

    // 0 or negative max values don't make sense, so we return a 0 for those.
    if (maxExclusive > 0) {
        result = randomNext(random) % maxExclusive;
    }

    return result;
}

s32 randomBetween(Random* random, s32 minInclusive, s32 maxExclusive)
{
    s32 result = minInclusive;

    // 0 or negative max values don't make sense, so we return a 0 for those.
    if (maxExclusive > minInclusive) {
        s32 range = maxExclusive - minInclusive;
        result = minInclusive + (randomNext(random) % range);
    }

    return result;
}

bool randomBool(Random* random)
{
    return (randomNext(random) % 2) != 0;
}

float randomFloatBetween(Random* random, float minInclusive, float maxExclusive)
{
    float zeroToOne = randomFloat01(random);

    return (zeroToOne * (maxExclusive - minInclusive)) + minInclusive;
}

float randomFloat01(Random* random)
{
    s32 intValue = randomNext(random);
    float result = abs_float(((float)intValue) / ((float)s32Max));
    return result;
}

Rect2I randomlyPlaceRectangle(Random* random, V2I size, Rect2I boundary)
{
    Rect2I result = irectXYWH(
        randomBetween(random, boundary.x, boundary.x + boundary.w - size.x),
        randomBetween(random, boundary.y, boundary.y + boundary.h - size.y),
        size.x, size.y);
    return result;
}

//
// Noise
//
void generate1DNoise(Random* random, Array<float>* destination, s32 smoothingPasses, bool wrap)
{
    for (s32 i = 0; i < destination->count; i++) {
        (*destination)[i] = randomFloat01(random);
    }

    // Smoothing
    for (s32 iteration = 0; iteration < smoothingPasses; iteration++) {
        if (wrap) {
            // Front
            (*destination)[0] = ((*destination)[destination->count - 1] + (*destination)[0] + (*destination)[1]) / 3.0f;
            // Back
            (*destination)[destination->count - 1] = ((*destination)[destination->count - 2] + (*destination)[destination->count - 1] + (*destination)[0]) / 3.0f;
        } else {
            // Fake normalisation because otherwise the ends get weird

            // Front
            (*destination)[0] = ((*destination)[0] + (*destination)[1]) / 2.0f;
            // Back
            (*destination)[destination->count - 1] = ((*destination)[destination->count - 2] + (*destination)[destination->count - 1]) / 2.0f;
        }

        for (s32 i = 1; i < destination->count - 1; i++) {
            (*destination)[i] = ((*destination)[i - 1] + (*destination)[i] + (*destination)[i + 1]) / 3.0f;
        }
    }
}

//
// Mersenne Twister
//

void MT_randomSeed(RandomMT* random, s32 seed)
{
    random->index = 624;
    random->mt[0] = seed;
    for (int i = 1; i < 624; i++) {
        random->mt[i] = (s32)(1812433253 * (random->mt[i - 1] ^ random->mt[i - 1] >> 30) + i);
    }
}

void MT_randomTwist(RandomMT* random)
{
    for (int i = 0; i < 624; i++) {
        s32 y = (s32)((random->mt[i] & 0x80000000) + (random->mt[(i + 1) % 624] & 0x7fffffff));

        random->mt[i] = random->mt[(i + 397) % 624] ^ y >> 1;

        if (y % 2) {
            random->mt[i] = random->mt[i] ^ 0x9908b0df;
        }
    }

    random->index = 0;
}

s32 MT_randomNext(RandomMT* random)
{
    if (random->index >= 624) {
        MT_randomTwist(random);
    }

    s32 y = random->mt[random->index];

    y = y ^ (y >> 11);
    y = y ^ ((y << 7) & 2636928640);
    y = y ^ ((y << 15) & 4022730752);
    y = y ^ (y >> 18);

    random->index++;

    return y;
}
