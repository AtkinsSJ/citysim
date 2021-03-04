#pragma once

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

inline s32 abs_s32(s32 in)
{
	return (in < 0) ? -in : in;
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

inline f32 sqrt_f32(f32 in)
{
	return (f32) sqrt(in);
}

inline f32 abs_f32(f32 in)
{
	return (in < 0.0f) ? -in : in;
}

inline f32 fraction_f32(f32 in)
{
	return (f32) fmod(in, 1.0f);
}

inline f32 sin32(f32 radians)
{
	return (f32) sin(radians);
}

inline f32 cos32(f32 radians)
{
	return (f32) cos(radians);
}

inline f32 tan32(f32 radians)
{
	return (f32) tan(radians);
}

inline s32 divideCeil(s32 numerator, s32 denominator)
{
	return (numerator + denominator - 1) / denominator;
}

inline s32 truncate32(s64 in)
{
	ASSERT(in <= s32Max); //Value is too large to truncate to s32!
	return (s32) in;
}

template<typename T>
inline bool canCastIntTo(s64 input)
{
	bool result = (input >= minPossibleValue<T>() && input <= maxPossibleValue<T>());

	return result;
}

template<typename T>
inline T truncate(s64 in)
{
	ASSERT(canCastIntTo<T>(in));
	return (T) in;
}

inline f32 clamp01(f32 in)
{
	return clamp(in, 0.0f, 1.0f);
}

u8 clamp01AndMap_u8(f32 in)
{
	return (u8)(clamp01(in) * 255.0f);
}

template<typename T>
inline T clamp(T value, T min, T max)
{
	ASSERT(min <= max); //min > max in clamp()!
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

template<typename T>
inline T min(T a, T b)
{
	return (a < b) ? a : b;
}

template<typename T, typename... Args>
T min(T a, Args... args)
{
	T b = min(args...);
	return (a < b) ? a : b;
}

template<typename T>
inline T max(T a, T b)
{
	return (a > b) ? a : b;
}

template<typename T, typename... Args>
T max(T a, Args... args)
{
	T b = max(args...);
	return (a > b) ? a : b;
}

template<typename T>
inline T wrap(T value, T max)
{
	return (value + max) % max;
}

template<typename T>
inline T lerp(T a, T b, f32 position)
{
	return (T)(a + (b-a)*position);
}

template<typename T>
inline T approach(T currentValue, T targetValue, T distance)
{
	T result = currentValue;

	if (targetValue < currentValue)
	{
		result = max(currentValue - distance, targetValue);
	}
	else
	{
		result = min(currentValue + distance, targetValue);
	}

	return result;
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

inline bool equals(f32 a, f32 b, f32 epsilon)
{
	return (abs_f32(a-b) < epsilon);
}
