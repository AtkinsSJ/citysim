/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/ChunkedArray.h>
#include <Util/Rectangle.h>

struct DirtyRects {
    ChunkedArray<Rect2I> rects;
    s32 expansionRadius;
    Rect2I bounds;
};

// NB: rects added with markRectAsDirty() are expanded by `expansionRadius` automatically
void initDirtyRects(DirtyRects* dirtyRects, MemoryArena* arena, s32 expansionRadius = 0, Rect2I bounds = {});
void markRectAsDirty(DirtyRects* dirtyRects, Rect2I bounds);
void clearDirtyRects(DirtyRects* dirtyRects);
bool isDirty(DirtyRects* dirtyRects);

Rect2I getOverallRect(DirtyRects* dirtyRects);
