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

typedef SDL_Color Color;

const int null = 0;

struct Coord {
	int32 x,y;
};

inline Coord coord(int32 x, int32 y) {
	return {x,y};
}

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

inline V2 v2(Coord coord) {
	return {(real32)coord.x, (real32)coord.y};
}
inline V2 v2(real32 x, real32 y) {
	return {x,y};
}
inline V2 v2(int x, int y) {
	return {(real32)x, (real32)y};
}

inline bool inRect(Rect rect, V2 pos) {
	return pos.x >= rect.x
		&& pos.x < (rect.x + rect.w)
		&& pos.y >= rect.y
		&& pos.y < (rect.y + rect.h);
}

inline V2 centre(Rect *rect) {
	return v2(
		(real32)rect->x + (real32)rect->w / 2.0f,
		(real32)rect->y + (real32)rect->h / 2.0f
	);
}

inline real32 clamp(real32 value, real32 min, real32 max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

inline real32 v2Length(V2 v) {
	return sqrt(v.x*v.x + v.y*v.y);
}

inline V2 operator+(V2 lhs, V2 rhs) {
	V2 result;
	result.x = lhs.x + rhs.x;
	result.y = lhs.y + rhs.y;
	return result;
}
inline V2 operator+=(V2 &lhs, V2 rhs) {
	lhs = lhs + rhs;
	return lhs;
}
inline V2 operator-(V2 lhs, V2 rhs) {
	V2 result;
	result.x = lhs.x - rhs.x;
	result.y = lhs.y - rhs.y;
	return result;
}
inline V2 operator-=(V2 &lhs, V2 rhs) {
	lhs = lhs - rhs;
	return lhs;
}
inline V2 operator*(V2 v, real32 s) {
	V2 result;
	result.x = v.x * s;
	result.y = v.y * s;
	return result;
}
inline V2 operator*=(V2 &v, real32 s) {
	v = v * s;
	return v;
}

inline V2 limit(V2 vector, real32 maxLength) {
	real32 length = v2Length(vector);
	if (length > maxLength) {
		vector *= maxLength / length;
	}
	return vector;
}