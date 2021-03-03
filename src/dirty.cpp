#pragma once

void initDirtyRects(DirtyRects *dirtyRects, MemoryArena *arena, s32 expansionRadius, Rect2I bounds)
{
	*dirtyRects = {};
	initChunkedArray(&dirtyRects->rects, arena, 32);
	dirtyRects->expansionRadius = expansionRadius;
	dirtyRects->bounds = bounds;
}

void markRectAsDirty(DirtyRects *dirtyRects, Rect2I rect)
{
	bool added = false;

	Rect2I rectToAdd = expand(rect, dirtyRects->expansionRadius);

	if (areaOf(dirtyRects->bounds) > 0)
	{
		rectToAdd = intersect(rectToAdd, dirtyRects->bounds);
	}

	// Skip empty rects
	if (areaOf(rectToAdd) == 0)
	{
		return;
	}

	if (dirtyRects->rects.count > 128)
	{
		logWarn("Over 128 dirty rects, which is probably a bug? (Count: {0}) Skipping duplicate checks."_s, {formatInt(dirtyRects->rects.count)});
		DEBUG_BREAK();
	}
	else
	{
		// Check to see if this rectangle is contained by an existing dirty rect
		for (auto it = iterate(&dirtyRects->rects);
			hasNext(&it);
			next(&it))
		{
			Rect2I *existingRect = get(&it);

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
				hasNext(&it);
				next(&it))
			{
				Rect2I existingRect = getValue(&it);
				if (contains(rectToAdd, existingRect))
				{
					removeIndex(&dirtyRects->rects, getIndex(&it), false);
				}
			}
		}
	}

	if (!added)
	{
		dirtyRects->rects.append(rectToAdd);
	}
}

void clearDirtyRects(DirtyRects *dirtyRects)
{
	dirtyRects->rects.clear();
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
		result = *dirtyRects->rects.get(0);

		for (auto it = iterate(&dirtyRects->rects, 1, false);
			hasNext(&it);
			next(&it))
		{
			result = unionOf(result, getValue(&it));
		}
	}

	return result;
}
