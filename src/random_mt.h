// Mersenne Twister!

#pragma once

/**
 *	Don't include this directly, include random.h which wraps this or any other random number generator.
 */

struct RandomMT
{
	s32 mt[624];
	s32 index;
};

void MT_randomSeed(RandomMT *random, s32 seed)
{
	random->index = 624;
	random->mt[0] = seed;
	for (int i=1; i<624; i++)
	{
		random->mt[i] = (s32) (1812433253 * (random->mt[i-1] ^ random->mt[i-1] >> 30) + i);
	}
}

void MT_randomTwist(RandomMT *random)
{
	for (int i=0; i<624; i++)
	{
		s32 y = (s32)((random->mt[i] & 0x80000000) + (random->mt[(i + 1) % 624] & 0x7fffffff));

		random->mt[i] = random->mt[(i + 397) % 624] ^ y >> 1;

		if (y % 2)
		{
			random->mt[i] = random->mt[i] ^ 0x9908b0df;
		}
	}

	random->index = 0;
}

s32 MT_randomNext(RandomMT *random)
{
	if (random->index >= 624)
	{
		MT_randomTwist(random);
	}

	s32 y = random->mt[random->index];

	y = y ^ y >> 11;
	y = y ^ y << 7 & 2636928640;
	y = y ^ y << 15 & 4022730752;
	y = y ^ y >> 18;

    random->index++;

    return y;
}