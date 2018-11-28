#pragma once

// maths.h

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

// How far is the point from the rectangle? Returns 0 if the point is inside the rectangle.
inline s32 manhattanDistance(Rect2I rect, V2I point)
{
	s32 result = 0;

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

inline s32 manhattanDistance(Rect2I a, Rect2I b)
{
	s32 result = 0;

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

inline f32 lerp(f32 a, f32 b, f32 position)
{
	return a + (b-a)*position;
}

inline V2 lerp(V2 a, V2 b, f32 position)
{
	return a + (b-a)*position;
}

inline s32 clampToRangeWrapping(s32 minInclusive, s32 maxInclusive, s32 offset)
{
	s32 t = offset % (maxInclusive - minInclusive + 1);
	return minInclusive + t;
}
inline u32 clampToRangeWrapping(u32 minInclusive, u32 maxInclusive, u32 offset)
{
	u32 t = offset % (maxInclusive - minInclusive + 1);
	return minInclusive + t;
}

inline f32 moveTowards(f32 currentValue, f32 targetValue, f32 distance)
{
	f32 result = currentValue;

	if (targetValue < currentValue)
	{
		result = MAX(currentValue - distance, targetValue);
	}
	else
	{
		result = MIN(currentValue + distance, targetValue);
	}

	return result;
}