#pragma once

#include "random_mt.h"

typedef RandomMT Random;
#define randomSeed(...) MT_randomSeed(__VA_ARGS__)
#define randomNext(...) MT_randomNext(__VA_ARGS__)

s32 randomInRange(Random *random, s32 maxExclusive)
{
	return randomNext(random) % maxExclusive;
}