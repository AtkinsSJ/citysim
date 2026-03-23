/*
 * Copyright (c) 2016-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Rectangle.h>
#include <Util/Vector.h>

enum class DragType : u8 {
    Line,
    Rect
};

enum class DragResultOperation : u8 {
    Nothing,
    DoAction,
    ShowPreview,
};

struct DragResult {
    DragResultOperation operation;
    Rect2I dragRect;
};

class DragState {
public:
    DragState(DragType, V2I item_size);

    DragResult update(Rect2I bounds, V2I mouse_tile_pos, bool mouse_is_over_ui);
    Rect2I get_drag_area(Rect2I bounds) const;

private:
    DragType m_drag_type;
    V2I m_item_size;
    bool m_is_dragging { false };
    V2I m_mouse_drag_start_world_pos { 0, 0 };
    V2I m_mouse_drag_end_world_pos { 0, 0 };
};
