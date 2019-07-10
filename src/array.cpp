#pragma once


template<typename T>
void initialiseArray(Array<T> *a, s32 initialSize)
{
	a->items = (T*) allocateRaw(initialSize * sizeof(T));
	a->count = 0;
	a->maxCount = initialSize;
}

template<typename T>
void freeArray(Array<T> *a)
{
	free(a->items);
}

// Doesn't free the memory, just marks it as empty.
template<typename T>
void clear(Array<T> *a)
{
	a->count = 0;
}

template<typename T>
bool resize(Array<T> *a, s32 newSize)
{
	ASSERT(newSize > a->count); //Shrinking an Array is not supported!
	logInfo("Resizing an array.");

	T *newItems = (T*) realloc(a->items, newSize * sizeof(T));
	if (newItems)
	{
		a->items = newItems;
		a->maxCount = newSize;

		return true;
	}

	return false;
}

template<typename T>
void reserve(Array<T> *a, s32 freeSlots)
{
	s32 desiredSize = a->count + freeSlots;
	if (desiredSize > a->maxCount)
	{
		s32 newSize = max(desiredSize, a->maxCount * 2);
		resize(a, newSize);
	}
}

template<typename T>
T *appendUninitialised(Array<T> *a)
{
	if (a->count >= a->maxCount)
	{
		s32 newSize = (a->maxCount == 0) ? 16 : (a->maxCount * 2);
		bool succeeded = resize(a, newSize);
		ASSERT(succeeded); //Failed to make room in array!
	}

	T *result = a->items + a->count++;

	return result;
}

template<typename T>
inline T *append(Array<T> *a, T item)
{
	T *result = appendUninitialised(a);
	*result = item;
	return result;
}

template<typename T>
inline T *appendBlank(Array<T> *a)
{
	T *result = appendUninitialised(a);
	*result = {};
	return result;
}

template<typename T>
T * pointerTo(Array<T> *a, s32 index)
{
	ASSERT(index >=0 && index < a->count); //Index out of range!
	return a->items + index;
}
