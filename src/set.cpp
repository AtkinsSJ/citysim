#pragma once

template<typename T>
void initSet(Set<T> *set, MemoryArena *arena, bool (*areItemsEqual)(T *a, T *b))
{
	*set = {};
	initChunkedArray(&set->items, arena, 64); // 64 is an arbitrary choice!
	set->areItemsEqual = areItemsEqual;
}

template<typename T>
void add(Set<T> *set, T item)
{
	if (!contains(set, item))
	{
		append(&set->items, item);
	}
}

template<typename T>
bool contains(Set<T> *set, T item)
{
	bool result = false;

	for (auto it = iterate(&set->items); !it.isDone; next(&it))
	{
		if (set->areItemsEqual(&item, get(it)))
		{
			result = true;
			break;
		}
	}

	return result;
}
