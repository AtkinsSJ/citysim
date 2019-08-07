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
s32 randomInRange(Random *random, s32 maxExclusive);
bool randomBool(Random *random);

//
// Internal
//
void MT_randomSeed(RandomMT *random, s32 seed);
void MT_randomTwist(RandomMT *random);
s32 MT_randomNext(RandomMT *random);
