#pragma once

template<typename T>
void initSet(Set<T> *set, MemoryArena *arena)
{
	*set = {};
	initChunkedArray(&set->items, arena, 64); // 64 is an arbitrary choice!
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
		if (equals(item, getValue(it)))
		{
			result = true;
			break;
		}
	}

	return result;
}
