#pragma once

struct DirtyRects
{
	ChunkedArray<Rect2I> rects;
};

void initDirtyRects(DirtyRects *dirtyRects, MemoryArena *arena);
void markRectAsDirty(DirtyRects *dirtyRects, Rect2I bounds);
void clearDirtyRects(DirtyRects *dirtyRects);
bool isDirty(DirtyRects *dirtyRects);
