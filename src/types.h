#pragma once
/*
Meaningless waffling about code typing style:

I guess ideally the types would all be short and convenient:
u8, u16, u32, u64
s8, s16, s32, s64 (or would that be i-whatever? Maybe s is better to avoid confusion with vars named i)
f32, f64
v2, v3, v4
v2i, v3i, v4i
rect2, rect3
rect2i, rect3i

Capitalisation would then be: lowercase for types, MixedCaseForEverythingElse.
But, I'm kind of used to ThisCase for types, from Java.
And ThisCaseForFunctions() looks wrong to me.

JB does This_Stuff for structs,
this_stuff for vars and functions.
I kind of like that, but I'm not sure I like the mix of cases for basic and struct types?
Part of this is that he uses auto-typing a lot so longer type names isn't a problem.

Personally I don't mind CamelCaseStuff, I don't find it hard to read.

I guess my style is:
blah for basic types
BactrianCamelCase for struct types
dromedaryCamelCase for variables and functions

I will create shorter versions of the basic types though.

*/

// Note for later: variadic macros `#define(foo, ...)` expand the ... with `__VA_ARGS__`

#define GLUE_(a, b) a ## b
#define GLUE(a, b) GLUE_(a, b)
#define STRVAL_(a) #a
#define STRVAL(a) STRVAL_(a)

/**********************************************
	Asserts
 **********************************************/

// Not sure if I want them to print to the log, or just disappear entirely.
// Really janky assertion macro, yay
#define ASSERT(expr, msg, ...) if(!(expr)) {*(int *)0 = 0;}
#define INVALID_DEFAULT_CASE default: ASSERT(false, "Invalid default case."); break;

/*
	Defer macro, to run code at the end of a scope.

	USAGE: defer { do_some_stuff_later(); };

	From https://stackoverflow.com/a/42060129/1178345
*/
#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }
#define DEFER_(LINE) zz_defer##LINE
#define DEFER(LINE) DEFER_(LINE)
#define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif // defer

#include <stdint.h>
#include <float.h>

typedef int8_t  s8;
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

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

const u8  u8Max  = UINT8_MAX;
const u16 u16Max = UINT16_MAX;
const u32 u32Max = UINT32_MAX;
const u64 u64Max = UINT64_MAX;

typedef float  f32;
typedef double f64;

const f32 f32Min = -FLT_MAX;
const f32 f32Max =  FLT_MAX;
const f64 f64Min = -DBL_MAX;
const f64 f64Max =  DBL_MAX;

typedef intptr_t  smm;
//typedef uintptr_t umm; // Turned this off because I don't think there's a good reason for using it?

#include <uchar.h>
typedef char32_t unichar;

#define null nullptr

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

struct V2I {
	s32 x,y;
};

struct Rect2I {
	union {
		struct {V2I pos;};
		struct {s32 x, y;};
	};
	union {
		struct {V2I dim;};
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

struct Rect2 {
	union {
		V2 pos;
		struct {f32 x, y;};
	};
	union {
		V2 size;
		struct {f32 w,h;};
	};
};

/*****************************************************************
 * Somewhat less-hacky doubly-linked-list stuff using templates. *
 *****************************************************************/
template<typename T>
struct LinkedListNode
{
	T *prevNode;
	T *nextNode;
};

template<typename T>
void initLinkedListSentinel(T *sentinel)
{
	sentinel->prevNode = sentinel;
	sentinel->nextNode = sentinel;
}

template<typename T>
void addToLinkedList(T *item, T *sentinel)
{
	item->prevNode = sentinel->prevNode;
	item->nextNode = sentinel;
	item->prevNode->nextNode = item;
	item->nextNode->prevNode = item;
}

template<typename T>
void removeFromLinkedList(T *item)
{
	item->nextNode->prevNode = item->prevNode;
	item->prevNode->nextNode = item->nextNode;

	item->nextNode = item->prevNode = item;
}

template<typename T>
bool linkedListIsEmpty(LinkedListNode<T> *sentinel)
{
	bool hasNext = (sentinel->nextNode != sentinel);
	bool hasPrev = (sentinel->prevNode != sentinel);
	ASSERT(hasNext == hasPrev, "Linked list is corrupted!");
	return !hasNext;
}

template<typename T>
void moveAllNodes(T *srcSentinel, T *destSentinel)
{
	if (linkedListIsEmpty(srcSentinel)) return;

	while (!linkedListIsEmpty(srcSentinel))
	{
		T *node = srcSentinel->nextNode;
		removeFromLinkedList(node);
		addToLinkedList(node, destSentinel);
	}

	srcSentinel->nextNode = srcSentinel->prevNode = srcSentinel;
}

/**********************************************
	General
 **********************************************/

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define WRAP(value, max) (((value) + (max)) % (max))

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

// Does a byte-by-byte comparison of the two structs, so ANY difference will show up!
// In other cases, you'll want to write a type-specific function.
// I'm not entirely confident this will work for all types, so make sure to TEST with any types you use it for!
template<class T>
bool equals(T a, T b)
{
	bool areEqual = true;

	u8 *pA = (u8*)(&a);
	u8 *pB = (u8*)(&b);
	smm byteSize = sizeof(T);

	for (smm i=0; i<byteSize; i++, pA++, pB++)
	{
		if (*pA != *pB)
		{
			areEqual = false;
			break;
		}
	}

	return areEqual;
}

bool equals(f32 a, f32 b, f32 epsilon)
{
	return (fabs(a-b) < epsilon);
}