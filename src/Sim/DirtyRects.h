/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/ChunkedArray.h>
#include <Util/Rectangle.h>

class DirtyRects {
public:
    DirtyRects() = default; // FIXME: Temporary until users are initialized properly.
    DirtyRects(MemoryArena&, s32 expansion_radius, Optional<Rect2I> bounds = {});

    void mark_dirty(Rect2I);
    void clear();
    bool is_dirty() const;

    ChunkedArray<Rect2I> const& rects() const { return m_rects; }
    Rect2I combined_dirty_rect() const;

private:
    Optional<Rect2I> m_bounds;
    s32 m_expansion_radius;
    ChunkedArray<Rect2I> m_rects;
};
