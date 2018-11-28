#pragma once

template<class T> struct Array {
	s32 count;
	s32 maxCount;
	T *items;

	Array(s32 initialSize)
	{
		items = (T*) calloc(initialSize, sizeof(T));
		count = 0;
		maxCount = initialSize;
	}

	~Array()
	{
		free(items);
	}

	T operator[](int index)
	{
		ASSERT(index >=0 && index < count, "Index out of range!");
		return items[index];
	}
};

template<class T>
bool resize(Array<T> *a, s32 newSize)
{
	ASSERT(newSize > a->count, "Shrinking an Array is not supported!");

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