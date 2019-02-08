#pragma once

// maths.h
#include <math.h>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

// Standard rounding functions return doubles, so here's some int ones.
inline s32 round_s32(f32 in)
{
	return (s32) round(in);
}

inline s32 floor_s32(f32 in)
{
	return (s32) floor(in);
}

inline s32 ceil_s32(f32 in)
{
	return (s32) ceil(in);
}

inline f32 round_f32(f32 in)
{
	return (f32) round(in);
}

inline f32 floor_f32(f32 in)
{
	return (f32) floor(in);
}

inline f32 ceil_f32(f32 in)
{
	return (f32) ceil(in);
}

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

inline Rect2I confineRectangle(Rect2I inner, Rect2I outer)
{
	Rect2I result = inner;

	// X
	if (result.w > outer.w)
	{
		// If it's too big, centre it.
		result.x = outer.x - ((result.w - outer.w) / 2);
	}
	else if (result.x < outer.x)
	{
		result.x = outer.x;
	}
	else if ((result.x + result.w) > (outer.x + outer.w))
	{
		result.x = outer.x + outer.w - result.w;
	}

	// Y
	if (result.h > outer.h)
	{
		// If it's too big, centre it.
		result.y = outer.y - ((result.h - outer.h) / 2);
	}
	else if (result.y < outer.y)
	{
		result.y = outer.y;
	}
	else if ((result.y + result.h) > (outer.y + outer.h))
	{
		result.y = outer.y + outer.h - result.h;
	}

	return result;
}

inline Rect2I centreRectangle(Rect2I inner, Rect2I outer)
{
	Rect2I result = inner;

	result.x = outer.x - ((result.w - outer.w) / 2);
	result.y = outer.y - ((result.h - outer.h) / 2);

	return result;
}