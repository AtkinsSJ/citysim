#pragma once

/*
 * TODO: Better control over where the data comes from.
 * Not sure how to do that with allowing resizing though. Maybe we don't?
 * Could have a fixed-size option too, that allocates from a certain arena.
 */

template<class T>
struct Array
{
	u32 count;
	u32 maxCount;
	T *items;

	T operator[](u32 index)
	{
		ASSERT(index >=0 && index < count, "Index out of range!");
		return items[index];
	}
};

template<class T>
void initialiseArray(Array<T> *a, u32 initialSize)
{
	a->items = (T*) calloc(initialSize, sizeof(T));
	a->count = 0;
	a->maxCount = initialSize;
}

template<class T>
void free(Array<T> *a)
{
	free(a->items);
}

// Doesn't free the memory, just marks it as empty.
template<class T>
void clear(Array<T> *a)
{
	a->count = 0;
}

template<class T>
bool resize(Array<T> *a, u32 newSize)
{
	ASSERT(newSize > a->count, "Shrinking an Array is not supported!");
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

template<class T>
T *append(Array<T> *a, T item)
{
	if (a->count >= a->maxCount)
	{
		ASSERT(resize(a, a->maxCount * 2), "Failed to make room in array!");
	}

	T *result = a->items + a->count++;

	*result = item;

	return result;
}


template<class T>
T *appendBlank(Array<T> *a)
{
	if (a->count >= a->maxCount)
	{
		ASSERT(resize(a, a->maxCount * 2), "Failed to make room in array!");
	}

	T *result = a->items + a->count++;

	return result;
}

template<class T>
T * pointerTo(Array<T> *a, u32 index)
{
	ASSERT(index >=0 && index < a->count, "Index out of range!");
	return a->items + index;
}