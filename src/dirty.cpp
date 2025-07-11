/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "dirty.h"

void initDirtyRects(DirtyRects* dirtyRects, MemoryArena* arena, s32 expansionRadius, Rect2I bounds)
{
    *dirtyRects = {};
    initChunkedArray(&dirtyRects->rects, arena, 32);
    dirtyRects->expansionRadius = expansionRadius;
    dirtyRects->bounds = bounds;
}

void markRectAsDirty(DirtyRects* dirtyRects, Rect2I rect)
{
    bool added = false;

    Rect2I rectToAdd = expand(rect, dirtyRects->expansionRadius);

    if (areaOf(dirtyRects->bounds) > 0) {
        rectToAdd = intersect(rectToAdd, dirtyRects->bounds);
    }

    // Skip empty rects
    if (areaOf(rectToAdd) == 0) {
        return;
    }

    if (dirtyRects->rects.count > 128) {
        logWarn("Over 128 dirty rects, which is probably a bug? (Count: {0}) Skipping duplicate checks."_s, { formatInt(dirtyRects->rects.count) });
        DEBUG_BREAK();
    } else {
        // Check to see if this rectangle is contained by an existing dirty rect
        for (auto it = dirtyRects->rects.iterate();
            it.hasNext();
            it.next()) {
            Rect2I* existingRect = it.get();

            if (contains(*existingRect, rectToAdd)) {
                added = true;
                break;
            }
        }

        // Remove any existing rects that are inside our new one
        if (!added) {
            for (auto it = dirtyRects->rects.iterateBackwards();
                it.hasNext();
                it.next()) {
                Rect2I existingRect = it.getValue();
                if (contains(rectToAdd, existingRect)) {
                    dirtyRects->rects.removeIndex(it.getIndex(), false);
                }
            }
        }
    }

    if (!added) {
        dirtyRects->rects.append(rectToAdd);
    }
}

void clearDirtyRects(DirtyRects* dirtyRects)
{
    dirtyRects->rects.clear();
}

 bool isDirty(DirtyRects* dirtyRects)
{
    return dirtyRects->rects.count > 0;
}

Rect2I getOverallRect(DirtyRects* dirtyRects)
{
    Rect2I result = irectXYWH(0, 0, 0, 0);

    if (dirtyRects->rects.count > 0) {
        result = *dirtyRects->rects.get(0);

        for (auto it = dirtyRects->rects.iterate(1, false);
            it.hasNext();
            it.next()) {
            result = unionOf(result, it.getValue());
        }
    }

    return result;
}
