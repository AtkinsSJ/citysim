#pragma once

template<typename T>
void initSet(Set<T> *set, MemoryArena *arena, bool (*areItemsEqual)(T *a, T *b))
{
	*set = {};
	initChunkedArray(&set->items, arena, 64); // 64 is an arbitrary choice!
	set->areItemsEqual = areItemsEqual;
}

template<typename T>
bool Set<T>::add(T item)
{
	bool didAdd = false;

	if (!contains(item))
	{
		items.append(item);
		didAdd = true;
	}

	return didAdd;
}

template <typename T>
bool Set<T>::remove(T item)
{
	bool didRemove = false;

	s32 removed = items.removeAll([&](T *t) {return areItemsEqual(&item, t); }, 1);
	didRemove = (removed == 1);

	return didRemove;
}

template<typename T>
bool Set<T>::contains(T item)
{
	bool result = false;

	for (auto it = items.iterate(); it.hasNext(); it.next())
	{
		if (areItemsEqual(&item, it.get()))
		{
			result = true;
			break;
		}
	}

	return result;
}

template <typename T>
bool Set<T>::isEmpty()
{
	return items.isEmpty();
}

template<typename T>
void Set<T>::clear()
{
	items.clear();
}

template<typename T>
SetIterator<T> Set<T>::iterate()
{
	SetIterator<T> result = {};
	result.itemsIterator = items.iterate();

	return result;
}

template<typename T>
Array<T> Set<T>::asSortedArray(bool (*compare)(T a, T b))
{
	Array<T> result = makeEmptyArray<T>();

	if (!isEmpty())
	{
		result = allocateArray<T>(tempArena, items.count, false);

		// Gather
		for (auto it = iterate(); it.hasNext(); it.next())
		{
			result.append(it.getValue());
		}

		// Sort
		result.sort(compare);
	}

	return result;
}

//////////////////////////////////////////////////
// ITERATOR STUFF                               //
//////////////////////////////////////////////////

template<typename T>
void SetIterator<T>::next()
{
	itemsIterator.next();
}

template<typename T>
bool SetIterator<T>::hasNext()
{
	return !itemsIterator.isDone;
}

template<typename T>
T *SetIterator<T>::get()
{
	return itemsIterator.get();
}

template<typename T>
T SetIterator<T>::getValue()
{
	return itemsIterator.getValue();
}