
void initRandom(Random *random, RandomType type, s32 seed)
{
	random->type = type;

	switch (type)
	{
		case Random_MT: {
			MT_randomSeed(&random->mt, seed);
		} break;

		INVALID_DEFAULT_CASE;
	}
}

s32 randomNext(Random *random)
{
	s32 result = 0;

	switch (random->type)
	{
		case Random_MT: {
			result = MT_randomNext(&random->mt);
		} break;

		INVALID_DEFAULT_CASE;
	}

	return result;
}

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

inline bool randomBool(Random *random)
{
	return (randomNext(random) % 2) != 0;
}

//
// Mersenne Twister
//

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
