#pragma once

template<typename T>
void initSet(Set<T> *set, MemoryArena *arena, bool (*areItemsEqual)(T *a, T *b))
{
	*set = {};
	initChunkedArray(&set->items, arena, 64); // 64 is an arbitrary choice!
	set->areItemsEqual = areItemsEqual;
}

template<typename T>
bool add(Set<T> *set, T item)
{
	bool didAdd = false;

	if (!contains(set, item))
	{
		set->items.append(item);
		didAdd = true;
	}

	return didAdd;
}

template<typename T>
bool contains(Set<T> *set, T item)
{
	bool result = false;

	for (auto it = set->items.iterate(); hasNext(&it); next(&it))
	{
		if (set->areItemsEqual(&item, get(&it)))
		{
			result = true;
			break;
		}
	}

	return result;
}

template<typename T>
void clear(Set<T> *set)
{
	set->items.clear();
}

template<typename T>
SetIterator<T> Set<T>::iterate()
{
	SetIterator<T> result = {};
	result.itemsIterator = items.iterate();

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