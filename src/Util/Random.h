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

enum class RandomType : u8 {
    MT,
};

// Mersenne Twister
struct RandomMT {
    s32 mt[624];
    s32 index;
};

// FIXME: Random as an abstract class and MT as a subclass
struct Random {
    RandomType type;
    union {
        RandomMT mt;
    };
};

void initRandom(Random* random, RandomType type, s32 seed);

s32 randomNext(Random* random);

s32 randomBelow(Random* random, s32 maxExclusive);
s32 randomBetween(Random* random, s32 minInclusive, s32 maxExclusive);

template<typename T>
T randomInRange(Random* random)
{
    return (T)randomBetween(random, minPossibleValue<T>(), maxPossibleValue<T>() + 1);
}

bool randomBool(Random* random);
float randomFloatBetween(Random* random, float minInclusive, float maxExclusive);
float randomFloat01(Random* random);
Rect2I randomlyPlaceRectangle(Random* random, V2I size, Rect2I boundary);

//
// Noise
//
void generate1DNoise(Random* random, Array<float>* destination, s32 smoothingPasses, bool wrap = false);

//
// Internal
//
void MT_randomSeed(RandomMT* random, s32 seed);
void MT_randomTwist(RandomMT* random);
s32 MT_randomNext(RandomMT* random);
