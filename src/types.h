#pragma once

#include <stdint.h>
#include <string>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

const int8 int8Min = INT8_MIN;
const int8 int8Max = INT8_MAX;
const int16 int16Min = INT16_MIN;
const int16 int16Max = INT16_MAX;
const int32 int32Min = INT32_MIN;
const int32 int32Max = INT32_MAX;
const int64 int64Min = INT64_MIN;
const int64 int64Max = INT64_MAX;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

const uint8 uint8Max = UINT8_MAX;
const uint16 uint16Max = UINT16_MAX;
const uint32 uint32Max = UINT32_MAX;
const uint64 uint64Max = UINT64_MAX;

typedef intptr_t smm;
typedef uintptr_t umm;

typedef float real32;
typedef double real64;

const real32 real32Min = -FLT_MAX;
const real32 real32Max = FLT_MAX;
const real64 real64Min = -DBL_MAX;
const real64 real64Max = DBL_MAX;

typedef std::string string;

const int null = 0;

enum Alignment {
	ALIGN_LEFT = 1,
	ALIGN_H_CENTRE = 2,
	ALIGN_RIGHT = 4,

	ALIGN_H = ALIGN_LEFT | ALIGN_H_CENTRE | ALIGN_RIGHT,

	ALIGN_TOP = 8,
	ALIGN_V_CENTRE = 16,
	ALIGN_BOTTOM = 32,
	
	ALIGN_V = ALIGN_TOP | ALIGN_V_CENTRE | ALIGN_BOTTOM,

	ALIGN_CENTRE = ALIGN_H_CENTRE | ALIGN_V_CENTRE,
};

struct Coord {
	int32 x,y;
};

struct Rect {
	union {
		struct {Coord pos;};
		struct {int32 x, y;};
	};
	union {
		struct {Coord dim;};
		struct {int32 w, h;};
	};
};

struct V2 {
	real32 x,y;
};

struct V3 {
	union {
		struct {
			real32 x,y,z;
		};
		struct {
			V2 xy;
			real32 z;
		};
	};
};

struct V4 {
	union {
		struct {
			V3 xyz;
			real32 w;
		};
		struct {
			V2 xy;
			real32 z, w;
		};
		struct {
			real32 x,y,z,w;
		};
		struct {
			real32 r,g,b,a;
		};
	};
};

struct RealRect {
	union {
		V2 pos;
		struct {real32 x, y;};
	};
	union {
		V2 size;
		struct {real32 w,h;};
	};
};

#include "matrix4.h"

#define DLinkedListMembers(type) type *prev; type *next;
#define DLinkedListInit(sentinel) (sentinel)->prev = (sentinel)->next = (sentinel);
#define DLinkedListInsertBefore(item, sentinel) \
	(item)->prev = (sentinel).prev;             \
	(item)->next = &(sentinel);                 \
	(item)->prev->next = (item);                \
	(item)->next->prev = (item);

/**********************************************
	General
 **********************************************/

#define MIN(a,b) ((a) < (b)) ? (a) : (b)
#define MAX(a,b) ((a) > (b)) ? (a) : (b)

inline real32 clamp(real32 value, real32 min, real32 max)
{
	ASSERT(min < max, "min > max in clamp()!");
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

/**********************************************
	COORD
 **********************************************/

inline Coord coord(int32 x, int32 y)
{
	return {x,y};
}
inline Coord coord(V2 v2)
{
	return {(int32)v2.x, (int32)v2.y};
}

inline Coord operator+(Coord lhs, Coord rhs)
{
	Coord result;
	result.x = lhs.x + rhs.x;
	result.y = lhs.y + rhs.y;
	return result;
}
inline Coord operator+=(Coord &lhs, Coord rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
inline Coord operator-(Coord lhs, Coord rhs)
{
	Coord result;
	result.x = lhs.x - rhs.x;
	result.y = lhs.y - rhs.y;
	return result;
}
inline Coord operator-=(Coord &lhs, Coord rhs)
{
	lhs = lhs - rhs;
	return lhs;
}
inline Coord operator*(Coord v, int32 s)
{
	Coord result;
	result.x = v.x * s;
	result.y = v.y * s;
	return result;
}
inline Coord operator*=(Coord &v, int32 s)
{
	v = v * s;
	return v;
}
inline Coord operator/(Coord v, int32 s)
{
	Coord result;
	result.x = v.x / s;
	result.y = v.y / s;
	return result;
}
inline Coord operator/=(Coord &v, int32 s)
{
	v = v / s;
	return v;
}

/**********************************************
	V2
 **********************************************/

inline V2 v2(Coord coord)
{
	return {(real32)coord.x, (real32)coord.y};
}
inline V2 v2(real32 x, real32 y)
{
	return {x,y};
}
inline V2 v2(int x, int y)
{
	return {(real32)x, (real32)y};
}

inline real32 v2Length(V2 v)
{
	return sqrt(v.x*v.x + v.y*v.y);
}

inline V2 operator+(V2 lhs, V2 rhs)
{
	V2 result;
	result.x = lhs.x + rhs.x;
	result.y = lhs.y + rhs.y;
	return result;
}
inline V2 operator+=(V2 &lhs, V2 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
inline V2 operator-(V2 lhs, V2 rhs)
{
	V2 result;
	result.x = lhs.x - rhs.x;
	result.y = lhs.y - rhs.y;
	return result;
}
inline V2 operator-=(V2 &lhs, V2 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}
inline V2 operator*(V2 v, real32 s)
{
	V2 result;
	result.x = v.x * s;
	result.y = v.y * s;
	return result;
}
inline V2 operator*=(V2 &v, real32 s)
{
	v = v * s;
	return v;
}
inline V2 operator/(V2 v, real32 s)
{
	V2 result;
	result.x = v.x / s;
	result.y = v.y / s;
	return result;
}
inline V2 operator/=(V2 &v, real32 s)
{
	v = v / s;
	return v;
}

inline V2 limit(V2 vector, real32 maxLength)
{
	real32 length = v2Length(vector);
	if (length > maxLength)
	{
		vector *= maxLength / length;
	}
	return vector;
}

/**********************************************
	V3
 **********************************************/

inline V3 v3(real32 x, real32 y, real32 z)
{
	V3 v = {};
	v.x = x;
	v.y = y;
	v.z = z;

	return v;
}

inline real32 v3Length(V3 v)
{
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

inline V3 operator+(V3 lhs, V3 rhs)
{
	V3 result;
	result.x = lhs.x + rhs.x;
	result.y = lhs.y + rhs.y;
	result.z = lhs.z + rhs.z;
	return result;
}
inline V3 operator+=(V3 &lhs, V3 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
inline V3 operator-(V3 lhs, V3 rhs)
{
	V3 result;
	result.x = lhs.x - rhs.x;
	result.y = lhs.y - rhs.y;
	result.z = lhs.z - rhs.z;
	return result;
}
inline V3 operator-=(V3 &lhs, V3 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}
inline V3 operator*(V3 v, real32 s)
{
	V3 result;
	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	return result;
}
inline V3 operator*=(V3 &v, real32 s)
{
	v = v * s;
	return v;
}
inline V3 operator/(V3 v, real32 s)
{
	V3 result;
	result.x = v.x / s;
	result.y = v.y / s;
	result.z = v.z / s;
	return result;
}
inline V3 operator/=(V3 &v, real32 s)
{
	v = v / s;
	return v;
}

/**********************************************
	V4
 **********************************************/

inline V4 v4(real32 x, real32 y, real32 z, real32 w)
{
	V4 v = {};
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;

	return v;
}

inline V4 color255(uint8 r, uint8 g, uint8 b, uint8 a)
{
	V4 v = {};
	v.a = (real32)a / 255.0f;

	// NB: Prremultiplied alpha!
	v.r = v.a * ((real32)r / 255.0f);
	v.g = v.a * ((real32)g / 255.0f);
	v.b = v.a * ((real32)b / 255.0f);

	return v;
}
inline V4 makeWhite()
{
	return v4(1.0f,1.0f,1.0f,1.0f);
}

inline real32 v4Length(V4 v)
{
	return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

inline V4 operator+(V4 lhs, V4 rhs)
{
	V4 result;
	result.x = lhs.x + rhs.x;
	result.y = lhs.y + rhs.y;
	result.z = lhs.z + rhs.z;
	result.w = lhs.w + rhs.w;
	return result;
}
inline V4 operator+=(V4 &lhs, V4 rhs)
{
	lhs = lhs + rhs;
	return lhs;
}
inline V4 operator-(V4 lhs, V4 rhs)
{
	V4 result;
	result.x = lhs.x - rhs.x;
	result.y = lhs.y - rhs.y;
	result.z = lhs.z - rhs.z;
	result.w = lhs.w - rhs.w;
	return result;
}
inline V4 operator-=(V4 &lhs, V4 rhs)
{
	lhs = lhs - rhs;
	return lhs;
}
inline V4 operator*(V4 v, real32 s)
{
	V4 result;
	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	result.w = v.w * s;
	return result;
}
inline V4 operator*=(V4 &v, real32 s)
{
	v = v * s;
	return v;
}
inline V4 operator/(V4 v, real32 s)
{
	V4 result;
	result.x = v.x / s;
	result.y = v.y / s;
	result.z = v.z / s;
	result.w = v.w / s;
	return result;
}
inline V4 operator/=(V4 &v, real32 s)
{
	v = v / s;
	return v;
}

/**********************************************
	Rectangle (int)
 **********************************************/

inline Rect irectXYWH(int32 x, int32 y, int32 w, int32 h)
{
	Rect rect = {};
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	return rect;
}

inline Rect irectPosDim(Coord position, Coord dim)
{
	Rect rect = {};
	rect.pos = position;
	rect.dim = dim;
	return rect;
}

inline Rect irectCentreWH(Coord position, int32 w, int32 h)
{
	Rect rect = {};
	rect.x = position.x - w/2;
	rect.y = position.y - h/2;
	rect.w = w;
	rect.h = h;
	return rect;
}

inline Rect irectCentreDim(Coord position, Coord dim)
{
	Rect rect = {};
	rect.x = position.x - dim.x/2;
	rect.y = position.y - dim.y/2;
	rect.dim = dim;
	return rect;
}

inline Rect irectCovering(V2 a, V2 b)
{
	Rect rect = {};
	if (a.x < b.x)
	{
		rect.x = (int32)(a.x);
		rect.w = (int32)(b.x) - (int32)(a.x) + 1;
	} else {
		rect.x = (int32)(b.x);
		rect.w = (int32)(a.x+0.5f) - (int32)(b.x);
	}

	if (a.y < b.y)
	{
		rect.y = (int32)(a.y);
		rect.h = (int32)(b.y) - (int32)(a.y) + 1;
	} else {
		rect.y = (int32)(b.y);
		rect.h = (int32)(a.y+0.5f) - (int32)(b.y);
	}
	return rect;
}

inline bool inRect(Rect rect, Coord coord)
{
	return coord.x >= rect.x
		&& coord.x < (rect.x + rect.w)
		&& coord.y >= rect.y
		&& coord.y < (rect.y + rect.h);
}

inline bool inRect(Rect rect, V2 pos)
{
	return pos.x >= rect.x
		&& pos.x < (rect.x + rect.w)
		&& pos.y >= rect.y
		&& pos.y < (rect.y + rect.h);
}

inline bool rectInRect(Rect outer, Rect inner)
{
	return inner.x >= outer.x
		&& (inner.x + inner.w) <= (outer.x + outer.w)
		&& inner.y >= outer.y
		&& (inner.y + inner.h) <= (outer.y + outer.h);
}

inline Rect expandiRect(Rect rect, int32 addRadius)
{
	return irectXYWH(
		rect.x - addRadius,
		rect.y - addRadius,
		rect.w + (addRadius * 2),
		rect.h + (addRadius * 2)
	);
}

inline bool rectsOverlap(Rect a, Rect b)
{
	return (a.x < b.x + b.w)
		&& (b.x < a.x + a.w)
		&& (a.y < b.y + b.h)
		&& (b.y < a.y + a.h);
}

inline V2 centre(Rect rect)
{
	return v2(
		(real32)rect.x + (real32)rect.w / 2.0f,
		(real32)rect.y + (real32)rect.h / 2.0f
	);
}

inline Coord iCentre(Rect rect)
{
	return coord(rect.x + rect.w/2, rect.y + rect.h/2);
}

/**********************************************
	RealRect
 **********************************************/

inline RealRect realRect(V2 pos, real32 w, real32 h)
{
	RealRect rect = {};
	rect.pos = pos;
	rect.w = w;
	rect.h = h;
	return rect;
}

inline RealRect realRect(Rect intRect)
{
	RealRect rect = {};
	rect.x = (real32) intRect.x;
	rect.y = (real32) intRect.y;
	rect.w = (real32) intRect.w;
	rect.h = (real32) intRect.h;
	return rect;
}

inline RealRect rectXYWH(real32 x, real32 y, real32 w, real32 h)
{
	RealRect rect = {};
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	return rect;
}

inline RealRect rectCentreSize(V2 centre, V2 size)
{
	RealRect rect = {};
	rect.x = centre.x - size.x/2.0f;
	rect.y = centre.y - size.y/2.0f;
	rect.w = size.x;
	rect.h = size.y;
	return rect;
}

inline RealRect expandRect(RealRect rect, real32 addRadius)
{
	return rectXYWH(
		rect.x - addRadius,
		rect.y - addRadius,
		rect.w + (addRadius * 2.0f),
		rect.h + (addRadius * 2.0f)
	);
}

inline RealRect offset(RealRect rect, V2 offset)
{
	RealRect result = rect;
	result.pos += offset;

	return result;
}

inline bool inRect(RealRect rect, V2 pos)
{
	return pos.x >= rect.x
		&& pos.x < (rect.x + rect.w)
		&& pos.y >= rect.y
		&& pos.y < (rect.y + rect.h);
}

inline bool inRects(RealRect *rects, int32 rectCount, V2 pos)
{
	bool result = false;
	for (int i=0; i < rectCount; i++)
	{
		if (inRect(rects[i], pos))
		{
			result = true;
			break;
		}
	}
	return result;
}

inline V2 centre(RealRect rect)
{
	return v2(
		rect.x + rect.w / 2.0f,
		rect.y + rect.h / 2.0f
	);
}