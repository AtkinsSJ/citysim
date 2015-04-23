#pragma once

#include <stdint.h>

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

struct Coord {
	int32 x,y;
};

struct Rect {
	union {
		struct {Coord pos;};
		struct {uint32 x, y;};
	};
	uint32 w,h;
};

struct V2 {
	real32 x,y;
};

struct Color {
	uint8 r, g, b, a;
};

inline real32 clamp(real32 value, real32 min, real32 max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}