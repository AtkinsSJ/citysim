#pragma once

#include "util/Forward.h"

// Note for later: variadic macros `#define(foo, ...)` expand the ... with `__VA_ARGS__`

#define GLUE_(a, b) a##b
#define GLUE(a, b) GLUE_(a, b)
#define STRVAL_(a) #a
#define STRVAL(a) STRVAL_(a)

#define INVALID_DEFAULT_CASE \
    default:                 \
        ASSERT(false);       \
        break;

#include "util/Basic.h"

template<typename T>
bool equals(T a, T b);

#include <typeinfo>
template<typename T>
String typeNameOf();

enum Alignment {
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

enum class Orientation {
    Horizontal,
    Vertical
};

struct Padding {
    s32 top;
    s32 bottom;
    s32 left;
    s32 right;
};

template<typename T>
struct Maybe {
    bool isValid;
    T value;

    inline T orDefault(T defaultValue)
    {
        return isValid ? value : defaultValue;
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

template<typename T>
inline bool allAreValid(Maybe<T> input)
{
    return input.isValid;
}

template<typename T, typename... TS>
bool allAreValid(Maybe<T> first, Maybe<TS>... rest)
{
    return first.isValid && allAreValid(rest...);
}

template<typename T>
struct Indexed {
    T value;
    s32 index;
};

template<typename T>
inline Indexed<T> makeNullIndexedValue()
{
    Indexed<T> result;
    result.value = nullptr;
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
struct Array {
    s32 capacity;
    s32 count;
    T* items;

    // NB: it's a reference so you can do assignments!
    T& operator[](s32 index);
    T* get(s32 index); // Same as [] but easier when we're using an Array*
    T* first();
    T* last();

    T* append();
    T* append(T item);

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
Array<T> makeArray(s32 capacity, T* items, s32 count = 0);

template<typename T>
Array<T> makeEmptyArray();

//
// 2D Array
//

template<typename T>
struct Array2 {
    s32 w;
    s32 h;
    T* items;

    T& get(s32 x, s32 y);
    T getIfExists(s32 x, s32 y, T defaultValue);

    void set(s32 x, s32 y, T value);
};

template<typename T>
Array2<T> makeArray2(s32 w, s32 h, T* items);

template<typename T>
void fill(Array2<T>* array, T value);

template<typename T>
void fillRegion(Array2<T>* array, Rect2I region, T value);

//
// Colours
//
V4 color255(u8 r, u8 g, u8 b, u8 a);
V4 makeWhite();
V4 asOpaque(V4 color);
