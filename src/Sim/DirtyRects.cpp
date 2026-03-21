/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DirtyRects.h"

DirtyRects::DirtyRects(MemoryArena& arena, s32 expansion_radius, Optional<Rect2I> bounds)
    : m_bounds(move(bounds))
    , m_expansion_radius(expansion_radius)
{
    initChunkedArray(&m_rects, &arena, 32);
}

void DirtyRects::mark_dirty(Rect2I rect)
{
    bool added = false;

    Rect2I rectToAdd = rect.expanded(m_expansion_radius);

    if (m_bounds.has_value())
        rectToAdd = rectToAdd.intersected(m_bounds.value());

    // Skip empty rects
    if (!rectToAdd.has_positive_area())
        return;

    if (m_rects.count > 128) {
        logWarn("Over 128 dirty rects, which is probably a bug? (Count: {0}) Skipping duplicate checks."_s, { formatInt(m_rects.count) });
        DEBUG_BREAK();
    } else {
        // Check to see if this rectangle is contained by an existing dirty rect
        for (auto it = m_rects.iterate();
            it.hasNext();
            it.next()) {
            auto& existingRect = it.get();

            if (existingRect.contains(rectToAdd)) {
                added = true;
                break;
            }
        }

        // Remove any existing rects that are inside our new one
        if (!added) {
            for (auto it = m_rects.iterateBackwards();
                it.hasNext();
                it.next()) {
                Rect2I existingRect = it.getValue();
                if (rectToAdd.contains(existingRect)) {
                    m_rects.take_index(it.getIndex(), false);
                }
            }
        }
    }

    if (!added) {
        m_rects.append(rectToAdd);
    }
}

void DirtyRects::clear()
{
    m_rects.clear();
}

bool DirtyRects::is_dirty() const
{
    return m_rects.count > 0;
}

Rect2I DirtyRects::combined_dirty_rect() const
{
    Rect2I result { 0, 0, 0, 0 };

    if (m_rects.count > 0) {
        result = m_rects.get(0);

        for (auto it = m_rects.iterate(1, false);
            it.hasNext();
            it.next()) {
            result = result.union_with(it.getValue());
        }
    }

    return result;
}
