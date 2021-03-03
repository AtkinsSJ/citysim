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
