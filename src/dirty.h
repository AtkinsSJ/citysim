#pragma once

struct DirtyRects
{
	s32 expansionRadius;
	ChunkedArray<Rect2I> rects;
};

// NB: rects added with markRectAsDirty() are expanded by `expansionRadius` automatically
void initDirtyRects(DirtyRects *dirtyRects, MemoryArena *arena, s32 expansionRadius = 0);
void markRectAsDirty(DirtyRects *dirtyRects, Rect2I bounds);
void clearDirtyRects(DirtyRects *dirtyRects);
bool isDirty(DirtyRects *dirtyRects);

Rect2I getOverallRect(DirtyRects *dirtyRects);
