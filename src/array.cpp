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
		s32 newSize = (a->maxCount == 0) ? 16 : (a->maxCount * 2);
		ASSERT(resize(a, newSize), "Failed to make room in array!");
	}

	T *result = a->items + a->count++;

	*result = item;

	return result;
}

template<typename T>
T * pointerTo(Array<T> *a, s32 index)
{
	ASSERT(index >=0 && index < a->count, "Index out of range!");
	return a->items + index;
}

// NB: compare() should act like strcmp(): <0 if A comes before B, 0 if equal, >0 if B comes before A
template<typename T, typename CompareResult>
void sortInPlace(Array<T> *a, CompareResult (*compare)(T*, T*))
{
	DEBUG_FUNCTION();
	// This is an implementation of the 'comb sort' algorithm, low to high

	s32 gap = a->count;
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
		for (s32 i = 0;
			i + gap < a->count; // Here lies the remains of the flicker bug. It was <= not <. /fp
			i++)
		{
			T *first  = a->items + i;
			T *second = a->items + i+gap;
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
