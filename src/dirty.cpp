#pragma once

void initDirtyRects(DirtyRects *dirtyRects, MemoryArena *arena, s32 expansionRadius)
{
	*dirtyRects = {};
	initChunkedArray(&dirtyRects->rects, arena, 32);
	dirtyRects->expansionRadius = expansionRadius;
}

void markRectAsDirty(DirtyRects *dirtyRects, Rect2I bounds)
{
	bool added = false;

	Rect2I rectToAdd = expand(bounds, dirtyRects->expansionRadius);

	// Check to see if this rectangle is contained by an existing dirty rect
	for (auto it = iterate(&dirtyRects->rects);
		!it.isDone;
		next(&it))
	{
		Rect2I *existingRect = get(it);

		if (contains(*existingRect, rectToAdd))
		{
			added = true;
			break;
		}
	}

	// Remove any existing rects that are inside our new one
	if (!added)
	{
		for (auto it = iterateBackwards(&dirtyRects->rects);
			!it.isDone;
			next(&it))
		{
			Rect2I existingRect = getValue(it);
			if (contains(rectToAdd, existingRect))
			{
				removeIndex(&dirtyRects->rects, getIndex(it), false);
			}
		}
	}

	if (!added)
	{
		append(&dirtyRects->rects, rectToAdd);
	}
}

void clearDirtyRects(DirtyRects *dirtyRects)
{
	clear(&dirtyRects->rects);
}

inline bool isDirty(DirtyRects *dirtyRects)
{
	return dirtyRects->rects.count > 0;
}

Rect2I getOverallRect(DirtyRects *dirtyRects)
{
	Rect2I result = irectXYWH(0,0,0,0);

	if (dirtyRects->rects.count > 0)
	{
		result = *get(&dirtyRects->rects, 0);

		for (auto it = iterate(&dirtyRects->rects, 1, false);
			!it.isDone;
			next(&it))
		{
			result = unionOf(result, getValue(it));
		}
	}

	return result;
}
