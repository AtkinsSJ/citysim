#pragma once

#include <stdint.h>
#include <string>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

const s8  s8Min  = INT8_MIN;
const s8  s8Max  = INT8_MAX;
const s16 s16Min = INT16_MIN;
const s16 s16Max = INT16_MAX;
const s32 s32Min = INT32_MIN;
const s32 s32Max = INT32_MAX;
const s64 s64Min = INT64_MIN;
const s64 s64Max = INT64_MAX;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

const u8  u8Max  = UINT8_MAX;
const u16 u16Max = UINT16_MAX;
const u32 u32Max = UINT32_MAX;
const u64 u64Max = UINT64_MAX;

typedef intptr_t  smm;
typedef uintptr_t umm;

typedef float  f32;
typedef double f64;

const f32 f32Min = -FLT_MAX;
const f32 f32Max = FLT_MAX;
const f64 f64Min = -DBL_MAX;
const f64 f64Max = DBL_MAX;

// typedef std::string string;

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
	s32 x,y;
};

struct Rect {
	union {
		struct {Coord pos;};
		struct {s32 x, y;};
	};
	union {
		struct {Coord dim;};
		struct {s32 w, h;};
	};
};

struct V2 {
	f32 x,y;
};

struct V3 {
	union {
		struct {
			f32 x,y,z;
		};
		struct {
			V2 xy;
			f32 z;
		};
	};
};

struct V4 {
	union {
		struct {
			V3 xyz;
			f32 w;
		};
		struct {
			V2 xy;
			f32 z, w;
		};
		struct {
			f32 x,y,z,w;
		};
		struct {
			f32 r,g,b,a;
		};
	};
};

struct RealRect {
	union {
		V2 pos;
		struct {f32 x, y;};
	};
	union {
		V2 size;
		struct {f32 w,h;};
	};
};

#include "matrix4.h"
#include "array.h"

/**********************************************
	Slightly hacky doubly-linked list stuff
 **********************************************/

#define DLinkedListMembers(type) type *prev; type *next;
#define DLinkedListInit(sentinel) (sentinel)->prev = (sentinel)->next = (sentinel);
#define DLinkedListInsertBefore(item, sentinel) \
	(item)->prev = (sentinel)->prev;            \
	(item)->next = (sentinel);                  \
	(item)->prev->next = (item);                \
	(item)->next->prev = (item);
#define DLinkedListRemove(item)                 \
	(item)->next->prev = item->prev;            \
	(item)->prev->next = (item)->next;
#define DLinkedListIsEmpty(sentinel) (((sentinel)->prev == (sentinel)) && ((sentinel)->next == (sentinel)))
// TODO: Could shortcut this by simply changing the first and last list elements to point to the free list.
// Would be faster but more complicated, so maybe later if it matters.
#define DLinkedListFreeAll(type, srcSentinel, freeSentinel)   \
	{                                                         \
		type *toFree = (srcSentinel)->next;                   \
		while (toFree != (srcSentinel))                       \
		{                                                     \
			DLinkedListRemove(toFree);                        \
			DLinkedListInsertBefore(toFree, (freeSentinel));  \
			toFree = (srcSentinel)->next;                     \
		}                                                     \
	}

/**********************************************
	General
 **********************************************/

#define MIN(a,b) ((a) < (b)) ? (a) : (b)
#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#define WRAP(value, max) (((value) + (max)) % (max))

inline f32 clamp(f32 value, f32 min, f32 max)
{
	ASSERT(min < max, "min > max in clamp()!");
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

/**********************************************
	COORD
 **********************************************/

inline Coord coord(s32 x, s32 y)
{
	return {x,y};
}
inline Coord coord(V2 v2)
{
	return {(s32)v2.x, (s32)v2.y};
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
inline Coord operator*(Coord v, s32 s)
{
	Coord result;
	result.x = v.x * s;
	result.y = v.y * s;
	return result;
}
inline Coord operator*=(Coord &v, s32 s)
{
	v = v * s;
	return v;
}
inline Coord operator/(Coord v, s32 s)
{
	Coord result;
	result.x = v.x / s;
	result.y = v.y / s;
	return result;
}
inline Coord operator/=(Coord &v, s32 s)
{
	v = v / s;
	return v;
}

/**********************************************
	V2
 **********************************************/

inline V2 v2(Coord coord)
{
	return {(f32)coord.x, (f32)coord.y};
}
inline V2 v2(f32 x, f32 y)
{
	return {x,y};
}
inline V2 v2(int x, int y)
{
	return {(f32)x, (f32)y};
}

inline f32 v2Length(V2 v)
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
inline V2 operator*(V2 v, f32 s)
{
	V2 result;
	result.x = v.x * s;
	result.y = v.y * s;
	return result;
}
inline V2 operator*=(V2 &v, f32 s)
{
	v = v * s;
	return v;
}
inline V2 operator/(V2 v, f32 s)
{
	V2 result;
	result.x = v.x / s;
	result.y = v.y / s;
	return result;
}
inline V2 operator/=(V2 &v, f32 s)
{
	v = v / s;
	return v;
}

inline V2 limit(V2 vector, f32 maxLength)
{
	f32 length = v2Length(vector);
	if (length > maxLength)
	{
		vector *= maxLength / length;
	}
	return vector;
}

/**********************************************
	V3
 **********************************************/

inline V3 v3(f32 x, f32 y, f32 z)
{
	V3 v = {};
	v.x = x;
	v.y = y;
	v.z = z;

	return v;
}

inline f32 v3Length(V3 v)
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
inline V3 operator*(V3 v, f32 s)
{
	V3 result;
	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	return result;
}
inline V3 operator*=(V3 &v, f32 s)
{
	v = v * s;
	return v;
}
inline V3 operator/(V3 v, f32 s)
{
	V3 result;
	result.x = v.x / s;
	result.y = v.y / s;
	result.z = v.z / s;
	return result;
}
inline V3 operator/=(V3 &v, f32 s)
{
	v = v / s;
	return v;
}

/**********************************************
	V4
 **********************************************/

inline V4 v4(f32 x, f32 y, f32 z, f32 w)
{
	V4 v = {};
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;

	return v;
}

inline V4 color255(u8 r, u8 g, u8 b, u8 a)
{
	V4 v = {};
	v.a = (f32)a / 255.0f;

	// NB: Prremultiplied alpha!
	v.r = v.a * ((f32)r / 255.0f);
	v.g = v.a * ((f32)g / 255.0f);
	v.b = v.a * ((f32)b / 255.0f);

	return v;
}
inline V4 makeWhite()
{
	return v4(1.0f,1.0f,1.0f,1.0f);
}

inline f32 v4Length(V4 v)
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
inline V4 operator*(V4 v, f32 s)
{
	V4 result;
	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	result.w = v.w * s;
	return result;
}
inline V4 operator*=(V4 &v, f32 s)
{
	v = v * s;
	return v;
}
inline V4 operator/(V4 v, f32 s)
{
	V4 result;
	result.x = v.x / s;
	result.y = v.y / s;
	result.z = v.z / s;
	result.w = v.w / s;
	return result;
}
inline V4 operator/=(V4 &v, f32 s)
{
	v = v / s;
	return v;
}

/**********************************************
	Rectangle (int)
 **********************************************/

inline Rect irectXYWH(s32 x, s32 y, s32 w, s32 h)
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

inline Rect irectCentreWH(Coord position, s32 w, s32 h)
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
		rect.x = (s32)(a.x);
		rect.w = (s32)(b.x) - (s32)(a.x) + 1;
	} else {
		rect.x = (s32)(b.x);
		rect.w = (s32)(a.x+0.5f) - (s32)(b.x);
	}

	if (a.y < b.y)
	{
		rect.y = (s32)(a.y);
		rect.h = (s32)(b.y) - (s32)(a.y) + 1;
	} else {
		rect.y = (s32)(b.y);
		rect.h = (s32)(a.y+0.5f) - (s32)(b.y);
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

inline Rect expandiRect(Rect rect, s32 addRadius)
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
		(f32)rect.x + (f32)rect.w / 2.0f,
		(f32)rect.y + (f32)rect.h / 2.0f
	);
}

inline Coord iCentre(Rect rect)
{
	return coord(rect.x + rect.w/2, rect.y + rect.h/2);
}

/**********************************************
	RealRect
 **********************************************/

inline RealRect realRect(V2 pos, f32 w, f32 h)
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
	rect.x = (f32) intRect.x;
	rect.y = (f32) intRect.y;
	rect.w = (f32) intRect.w;
	rect.h = (f32) intRect.h;
	return rect;
}

inline RealRect rectXYWH(f32 x, f32 y, f32 w, f32 h)
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

inline RealRect expandRect(RealRect rect, f32 addRadius)
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

inline bool inRects(RealRect *rects, s32 rectCount, V2 pos)
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