#pragma once

enum RandomType
{
	Random_MT,
};

// Mersenne Twister
struct RandomMT
{
	s32 mt[624];
	s32 index;
};

struct Random
{
	RandomType type;
	union
	{
		RandomMT mt;
	};
};

void initRandom(Random *random, RandomType type, s32 seed);

s32 randomNext(Random *random);

s32 randomBelow(Random *random, s32 maxExclusive);
s32 randomBetween(Random *random, s32 minInclusive, s32 maxExclusive);

template<typename T>
T randomInRange(Random *random);

bool randomBool(Random *random);
f32 randomFloatBetween(Random *random, f32 minInclusive, f32 maxExclusive);
f32 randomFloat01(Random *random);
Rect2I randomlyPlaceRectangle(Random *random, V2I size, Rect2I boundary);

//
// Noise
//
void generate1DNoise(Random *random, Array<f32> *destination, s32 smoothingPasses, bool wrap=false);

//
// Internal
//
void MT_randomSeed(RandomMT *random, s32 seed);
void MT_randomTwist(RandomMT *random);
s32 MT_randomNext(RandomMT *random);
