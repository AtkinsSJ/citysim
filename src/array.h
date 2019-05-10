#pragma once

/*
 * TODO: Better control over where the data comes from.
 * Not sure how to do that with allowing resizing though. Maybe we don't?
 * Could have a fixed-size option too, that allocates from a certain arena.
 */

template<typename T>
struct Array
{
	u32 count;
	u32 maxCount;
	T *items;

	inline T operator[](u32 index)
	{
		ASSERT(index >=0 && index < count, "Index out of range!");
		return items[index];
	}
};

template<typename T>
void initialiseArray(Array<T> *a, u32 initialSize)
{
	a->items = (T*) allocateRaw(initialSize * sizeof(T));
	a->count = 0;
	a->maxCount = initialSize;
}

template<typename T>
void free(Array<T> *a)
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

template<typename T>
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

template<typename T>
T *appendBlank(Array<T> *a)
{
	if (a->count >= a->maxCount)
	{
		ASSERT(resize(a, a->maxCount * 2), "Failed to make room in array!");
	}

	T *result = a->items + a->count++;
	
	*result = {};

	return result;
}

template<typename T>
T * pointerTo(Array<T> *a, u32 index)
{
	ASSERT(index >=0 && index < a->count, "Index out of range!");
	return a->items + index;
}

// NB: compare() should act like strcmp(): <0 if A comes before B, 0 if equal, >0 if B comes before A
template<typename T, typename CompareResult>
void sortInPlace(Array<T> *a, CompareResult (*compare)(T*, T*))
{
	// This is an implementation of the 'comb sort' algorithm, low to high

	u32 gap = a->count;
	f32 shrink = 1.3f;

	bool swapped = false;

	while (gap > 1 || swapped)
	{
		gap = (u32)((f32)gap / shrink);
		if (gap < 1)
		{
			gap = 1;
		}

		swapped = false;

		// "comb" over the list
		for (u32 i = 0;
			i + gap < a->count; // Here lies the remains of the flicker bug. It was <= not <. /fp
			i++)
		{
			T *first  = pointerTo(a, i);
			T *second = pointerTo(a, i+gap);
			if (compare(first, second) > 0)
			{
				T temp = *first;
				*first = *second;
				*second = temp;

				swapped = true;
			}
		}
	}
}