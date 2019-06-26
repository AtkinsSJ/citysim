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

#define INVALID_DEFAULT_CASE default: ASSERT(false, "Invalid default case."); break;

/*
	Defer macro, to run code at the end of a scope.

	USAGE: defer { do_some_stuff_later(); };

	From https://stackoverflow.com/a/42060129/1178345
*/
#ifndef defer
struct defer_dummy {};
template <typename F> struct deferrer { F f; ~deferrer() { f(); } };
template <typename F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }
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

struct V2 {
	f32 x,y;
};

struct V2I {
	s32 x,y;
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

struct Rect2I {
	union {
		V2I pos;
		struct {s32 x, y;};
	};
	union {
		V2I size;
		struct {s32 w, h;};
	};
};

struct Matrix4 {
	union {
		f32 v[4][4]; // Column-major order, so [COLUMN][ROW]
		f32 flat[4*4];
	};
};


/**********************************************
	General
 **********************************************/


// Standard rounding functions return doubles, so here's some int ones.
s32 round_s32(f32 in);
s32 floor_s32(f32 in);
s32 ceil_s32(f32 in);
f32 round_f32(f32 in);
f32 floor_f32(f32 in);
f32 ceil_f32(f32 in);

s32 truncate32(s64 in);

s32 clamp(s32 value, s32 min, s32 max);
f32 clamp(f32 value, f32 min, f32 max);

s32 min(s32 a, s32 b);
s32 max(s32 a, s32 b);

s32 min(smm a, s32 b);
s32 max(smm a, s32 b);

f32 min(f32 a, f32 b);
f32 max(f32 a, f32 b);

u32 wrap(u32 value, u32 max);

//
// V2
//
V2 v2(f32 x, f32 y);
V2 v2(s32 x, s32 y);
V2 v2(V2I source);

f32 lengthOf(V2 v);
V2 limit(V2 vector, f32 maxLength);

V2 operator+(V2 lhs, V2 rhs);
V2 operator+=(V2 &lhs, V2 rhs);
V2 operator-(V2 lhs, V2 rhs);
V2 operator-=(V2 &lhs, V2 rhs);
V2 operator*(V2 v, f32 s);
V2 operator*=(V2 &v, f32 s);
V2 operator/(V2 v, f32 s);
V2 operator/=(V2 &v, f32 s);

//
// V2I
//
V2I v2i(s32 x, s32 y);
V2I v2i(V2 source);

f32 lengthOf(V2I v);

V2I operator+(V2I lhs, V2I rhs);
V2I operator+=(V2I &lhs, V2I rhs);
V2I operator-(V2I lhs, V2I rhs);
V2I operator-=(V2I &lhs, V2I rhs);
V2I operator*(V2I v, f32 s);
V2I operator*=(V2I &v, f32 s);
V2I operator/(V2I v, f32 s);
V2I operator/=(V2I &v, f32 s);

//
// V3
//
V3 v3(f32 x, f32 y, f32 z);
V3 v3(s32 x, s32 y, s32 z);

f32 lengthOf(V3 v);

V3 operator+(V3 lhs, V3 rhs);
V3 operator+=(V3 &lhs, V3 rhs);
V3 operator-(V3 lhs, V3 rhs);
V3 operator-=(V3 &lhs, V3 rhs);
V3 operator*(V3 v, f32 s);
V3 operator*=(V3 &v, f32 s);
V3 operator/(V3 v, f32 s);
V3 operator/=(V3 &v, f32 s);

//
// V4
//
V4 v4(f32 x, f32 y, f32 z, f32 w);
V4 v4(s32 x, s32 y, s32 z, s32 w);

f32 lengthOf(V4 v);

V4 operator+(V4 lhs, V4 rhs);
V4 operator+=(V4 &lhs, V4 rhs);
V4 operator-(V4 lhs, V4 rhs);
V4 operator-=(V4 &lhs, V4 rhs);
V4 operator*(V4 v, f32 s);
V4 operator*=(V4 &v, f32 s);
V4 operator/(V4 v, f32 s);
V4 operator/=(V4 &v, f32 s);

V4 color255(u8 r, u8 g, u8 b, u8 a);
V4 makeWhite();

//
// Rect2
//
Rect2 rectXYWH(f32 x, f32 y, f32 w, f32 h);
Rect2 rectXYWHi(s32 x, s32 y, s32 w, s32 h);
Rect2 rectPosSize(V2 pos, V2 size);
Rect2 rectCentreSize(V2 centre, V2 size);
Rect2 rectAligned(V2 origin, V2 size, u32 alignment);
Rect2 rect2(Rect2I source);

bool contains(Rect2 rect, f32 x, f32 y);
bool contains(Rect2 rect, V2 pos);
bool contains(Rect2 outer, Rect2 inner);
bool overlaps(Rect2 outer, Rect2 inner);

Rect2 expand(Rect2 rect, f32 radius);
Rect2 expand(Rect2 rect, f32 top, f32 right, f32 bottom, f32 left);
Rect2 intersect(Rect2 a, Rect2 b);
Rect2 intersectRelative(Rect2 outer, Rect2 inner);

V2 centreOf(Rect2 rect);
f32 areaOf(Rect2 rect);

V2 originWithinRectangle(Rect2 bounds, u32 alignment, f32 padding=0);

//
// Rect2I
//
Rect2I irectXYWH(s32 x, s32 y, s32 w, s32 h);
Rect2I irectNegativeInfinity();
Rect2I irectPosSize(V2I position, V2I size);
Rect2I irectCentreSize(V2I position, V2I size);
Rect2I irectMinMax(s32 xMin, s32 yMin, s32 xMax, s32 yMax);

bool contains(Rect2I rect, s32 x, s32 y);
bool contains(Rect2I rect, V2I pos);
bool contains(Rect2I outer, Rect2I inner);
bool overlaps(Rect2I outer, Rect2I inner);

Rect2I expand(Rect2I rect, s32 radius);
Rect2I expand(Rect2I rect, s32 top, s32 right, s32 bottom, s32 left);
Rect2I intersect(Rect2I a, Rect2I b);
Rect2I intersectRelative(Rect2I outer, Rect2I inner);

V2I centreOf(Rect2I rect);
s32 areaOf(Rect2I rect);

// Does a byte-by-byte comparison of the two structs, so ANY difference will show up!
// In other cases, you'll want to write a type-specific function.
// I'm not entirely confident this will work for all types, so make sure to TEST with any types you use it for!
template<typename T>
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
