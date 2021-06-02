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

#define INVALID_DEFAULT_CASE default: ASSERT(false); break;

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

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

const s8  s8Min  = INT8_MIN;
const s8  s8Max  = INT8_MAX;
const s16 s16Min = INT16_MIN;
const s16 s16Max = INT16_MAX;
const s32 s32Min = INT32_MIN;
const s32 s32Max = INT32_MAX;
const s64 s64Min = INT64_MIN;
const s64 s64Max = INT64_MAX;

const u8  u8Max  = UINT8_MAX;
const u16 u16Max = UINT16_MAX;
const u32 u32Max = UINT32_MAX;
const u64 u64Max = UINT64_MAX;

const f32 f32Min = -FLT_MAX;
const f32 f32Max =  FLT_MAX;
const f64 f64Min = -DBL_MAX;
const f64 f64Max =  DBL_MAX;

typedef intptr_t  smm;
//typedef uintptr_t umm; // Turned this off because I don't think there's a good reason for using it?

#include <uchar.h>
typedef char32_t unichar;

#define null nullptr

template<typename T>
bool equals(T a, T b);

#include <typeinfo>
template<typename T>
String typeNameOf();

struct V2;
struct V2I;
struct V3;
struct V4;
struct Rect2;
struct Rect2I;

enum Alignment
{
	ALIGN_LEFT = 1,
	ALIGN_H_CENTRE = 2,
	ALIGN_RIGHT = 4,
	ALIGN_EXPAND_H = 8,

	ALIGN_H = ALIGN_LEFT | ALIGN_H_CENTRE | ALIGN_RIGHT | ALIGN_EXPAND_H,

	ALIGN_TOP = 16,
	ALIGN_V_CENTRE = 32,
	ALIGN_BOTTOM = 64,
	ALIGN_EXPAND_V = 128,
	
	ALIGN_V = ALIGN_TOP | ALIGN_V_CENTRE | ALIGN_BOTTOM | ALIGN_EXPAND_V,

	ALIGN_CENTRE = ALIGN_H_CENTRE | ALIGN_V_CENTRE,
};

enum class Orientation
{
	Horizontal,
	Vertical
};

template<typename T>
struct Maybe
{
	bool isValid;
	T value;

	inline T orDefault(T default)
	{
		return isValid ? value : default;
	}
};

template<typename T>
inline Maybe<T> makeSuccess(T value)
{
	Maybe<T> result;
	result.isValid = true;
	result.value = value;

	return result;
}

template<typename T>
inline Maybe<T> makeFailure()
{
	Maybe<T> result = {};
	result.isValid = false;

	return result;
}

template <typename T>
inline bool allAreValid(Maybe<T> input)
{
	return input.isValid;
}

template <typename T, typename... TS>
bool allAreValid(Maybe<T> first, Maybe<TS>... rest)
{
	return first.isValid && allAreValid(rest...);
}

template<typename T>
struct Indexed
{
	T value;
	s32 index;
};

template<typename T>
inline Indexed<T> makeNullIndexedValue()
{
	Indexed<T> result;
	result.value = null;
	result.index = -1;

	return result;
}

template<typename T>
inline Indexed<T> makeIndexedValue(T value, s32 index)
{
	Indexed<T> result;
	result.value = value;
	result.index = index;

	return result;
}

//
// Array
//

template<typename T>
struct Array
{
	s32 capacity;
	s32 count;
	T *items;

	// NB: it's a reference so you can do assignments!
	T& operator[](s32 index);
	T* get(s32 index); // Same as [] but easier when we're using an Array*
	T* first();
	T* last();

	T *append();
	T *append(T item);

	bool isInitialised();
	bool isEmpty();
	bool hasRoom();

	void swap(s32 indexA, s32 indexB);

	// compareElements(T a, T b) -> returns (a < b), to sort low to high
	template<typename Comparison>
	void sort(Comparison compareElements);

	template<typename Comparison>
	void sortInternal(Comparison compareElements, s32 lowIndex, s32 highIndex);
};

template<typename T>
Array<T> makeArray(s32 capacity, T *items, s32 count = 0);

template<typename T>
Array<T> makeEmptyArray();

//
// 2D Array
//

template<typename T>
struct Array2
{
	s32 w;
	s32 h;
	T *items;

	T &get(s32 x, s32 y);
	T getIfExists(s32 x, s32 y, T defaultValue);

	void set(s32 x, s32 y, T value);
};

template<typename T>
Array2<T> makeArray2(s32 w, s32 h, T *items);

template<typename T>
void fill(Array2<T> *array, T value);

template<typename T>
void fillRegion(Array2<T> *array, Rect2I region, T value);

//
// Matrix4
//
// Matrices! Pretty sure I'm only going to use this for the camera projection,
// so certain things are missing - notably, we can only rotate in the z-axis
// because (for now) the game will be in 2D.
//
// TODO: When I move to 3D, implement full rotation of matrices!
//

struct Matrix4 {
	union {
		f32 v[4][4]; // Column-major order, so [COLUMN][ROW]
		f32 flat[4*4];
	};
};

Matrix4 identityMatrix4();
Matrix4 orthographicMatrix4(f32 left, f32 right, f32 top, f32 bottom, f32 nearClip, f32 farClip);
Matrix4 inverse(Matrix4 *source);
void translate(Matrix4 *matrix, V3 translation);
void scale(Matrix4 *matrix, V3 scale);
void rotateZ(Matrix4 *matrix, f32 radians);

Matrix4 operator*(Matrix4 a, Matrix4 b);
Matrix4 operator*=(Matrix4 &a, Matrix4 b);
V4 operator*(Matrix4 m, V4 v);

//
// Colours
//
V4 color255(u8 r, u8 g, u8 b, u8 a);
V4 makeWhite();
V4 asOpaque(V4 color);
