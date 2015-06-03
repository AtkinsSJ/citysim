#pragma once

#include <stdint.h>
#include <string>

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

typedef std::string string;

const int null = 0;

struct Coord {
	int32 x,y;
};

union Rect {
	struct {
		union {
			struct {Coord pos;};
			struct {int32 x, y;};
		};
		int32 w,h;
	};
	SDL_Rect sdl_rect;
};


inline bool inRect(Rect rect, Coord coord) {
	return coord.x >= rect.x
		&& coord.x < (rect.x + rect.w)
		&& coord.y >= rect.y
		&& coord.y < (rect.y + rect.h);
}

inline bool rectInRect(Rect outer, Rect inner) {
	return inner.x >= outer.x
		&& (inner.x + inner.w) <= (outer.x + outer.w)
		&& inner.y >= outer.y
		&& (inner.y + inner.h) <= (outer.y + outer.h);
}

struct V2 {
	real32 x,y;
};

typedef SDL_Color Color;

inline real32 clamp(real32 value, real32 min, real32 max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}