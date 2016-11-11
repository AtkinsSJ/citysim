#pragma once

// maths.h

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

// How far is the point from the rectangle? Returns 0 if the point is inside the rectangle.
inline int32 manhattanDistance(Rect rect, Coord point)
{
	int32 result = 0;

	if (point.x < rect.x)
	{
		result += rect.x - point.x;
	}
	else if (point.x >= (rect.x + rect.w))
	{
		result += 1 + point.x - (rect.x + rect.w);
	}

	if (point.y < rect.y)
	{
		result += rect.y - point.y;
	}
	else if (point.y >= (rect.y + rect.h))
	{
		result += 1 + point.y - (rect.y + rect.h);
	}

	return result;
}

inline int32 manhattanDistance(Rect a, Rect b)
{
	int32 result = 0;

	if (a.x + a.w <= b.x)
	{
		result += 1 + b.x - (a.x + a.w);
	}
	else if (b.x + b.w <= a.x)
	{
		result += 1 + a.x - (b.x + b.w);
	}

	if (a.y + a.h <= b.y)
	{
		result += 1 + b.y - (a.y + a.h);
	}
	else if (b.y + b.h <= a.y)
	{
		result += 1 + a.y - (b.y + b.h);
	}
	
	return result;
}

inline real32 lerp(real32 a, real32 b, real32 position)
{
	return a + (b-a)*position;
}

inline V2 lerp(V2 a, V2 b, real32 position)
{
	return a + (b-a)*position;
}

inline int32 clampToRangeWrapping(int32 minInclusive, int32 maxInclusive, int32 offset)
{
	int32 t = offset % (maxInclusive - minInclusive + 1);
	return minInclusive + t;
}