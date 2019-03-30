#pragma once

#include "random_mt.h"

typedef RandomMT Random;
#define randomSeed(...) MT_randomSeed(__VA_ARGS__)
#define randomNext(...) MT_randomNext(__VA_ARGS__)

s32 randomInRange(Random *random, s32 maxExclusive)
{
	s32 result = 0;

	// 0 or negative max values don't make sense, so we return a 0 for those.
	if (maxExclusive > 0)
	{
		result = randomNext(random) % maxExclusive;
	}

	return result;
}

bool randomBool(Random *random)
{
	return (randomNext(random) % 2) != 0;
}