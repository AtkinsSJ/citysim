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

typedef uint32 unichar;

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

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define WRAP(value, max) (((value) + (max)) % (max))

/**********************************************
	Asserts
 **********************************************/

// TODO: Make assertions behave differently when not in debug mode!
// Not sure if I want them to print to the log, or just disappear entirely.
// Really janky assertion macro, yay
#define ASSERT(expr, msg, ...) if(!(expr)) {SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, msg, ##__VA_ARGS__); *(int *)0 = 0;}
#define INVALID_DEFAULT_CASE default: ASSERT(false, "Invalid default case."); break;

#include "matrix4.h"
#include "array.h"