#pragma once

/*
 * TODO: Better control over where the data comes from.
 * Not sure how to do that with allowing resizing though. Maybe we don't?
 * Could have a fixed-size option too, that allocates from a certain arena.
 */

template<typename T>
struct Array
{
	s32 count;
	s32 maxCount;
	T *items;

	inline T operator[](s32 index)
	{
		ASSERT(index >=0 && index < count, "Index out of range!");
		return items[index];
	}
};

template<typename T>
void initialiseArray(Array<T> *a, s32 initialSize);

template<typename T>
void freeArray(Array<T> *a);

// Doesn't free the memory, just marks it as empty.
template<typename T>
void clear(Array<T> *a);

template<typename T>
bool resize(Array<T> *a, s32 newSize);

template<typename T>
T *append(Array<T> *a, T item);

template<typename T>
inline T *appendBlank(Array<T> *a)
{
	return append(a, {});
}

template<typename T>
T * pointerTo(Array<T> *a, s32 index);

// NB: compare() should act like strcmp(): <0 if A comes before B, 0 if equal, >0 if B comes before A
template<typename T, typename CompareResult>
void sortInPlace(Array<T> *a, CompareResult (*compare)(T*, T*));
