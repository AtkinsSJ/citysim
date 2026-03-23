/*
 * Copyright (c) 2016-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DragState.h"

#include <Debug/Debug.h>
#include <Input/Input.h>

DragState::DragState(DragType drag_type, V2I item_size)
    : m_drag_type(drag_type)
    , m_item_size(item_size)
{
}

DragResult DragState::update(Rect2I bounds, V2I mouse_tile_pos, bool mouse_is_over_ui)
{
    DEBUG_FUNCTION();

    DragResult result = {};

    if (m_is_dragging && mouseButtonJustReleased(MouseButton::Left)) {
        if (!mouse_is_over_ui) {
            result.operation = DragResultOperation::DoAction;
            result.dragRect = get_drag_area(bounds);
        }

        m_is_dragging = false;
    } else {
        // Update the dragging state
        if (!mouse_is_over_ui && mouseButtonJustPressed(MouseButton::Left)) {
            m_is_dragging = true;
            m_mouse_drag_start_world_pos = m_mouse_drag_end_world_pos = mouse_tile_pos;
        }

        if (mouseButtonPressed(MouseButton::Left) && m_is_dragging) {
            m_mouse_drag_end_world_pos = mouse_tile_pos;
            result.dragRect = get_drag_area(bounds);
        } else {
            result.dragRect = { mouse_tile_pos.x, mouse_tile_pos.y, 1, 1 };
        }

        // Preview
        if (!mouse_is_over_ui || m_is_dragging) {
            result.operation = DragResultOperation::ShowPreview;
        }
    }

    // minimum size
    if (result.dragRect.width() < m_item_size.x)
        result.dragRect.set_width(m_item_size.x);
    if (result.dragRect.height() < m_item_size.y)
        result.dragRect.set_height(m_item_size.y);

    return result;
}

Rect2I DragState::get_drag_area(Rect2I bounds) const
{
    DEBUG_FUNCTION();

    Rect2I result { 0, 0, 0, 0 };

    if (m_is_dragging) {
        switch (m_drag_type) {
        case DragType::Rect: {
            // This is more complicated than a simple rectangle covering both points.
            // If the user is dragging a 3x3 building, then the drag area must cover that 3x3 footprint
            // even if they drag right-to-left, and in the case where the rectangle is not an even multiple
            // of 3, the building placements are aligned to match that initial footprint.
            // I'm struggling to find a good way of describing that... But basically, we want this to be
            // as intuitive to use as possible, and that means there MUST be a building placed where the
            // ghost was before the user pressed the mouse button, no matter which direction or size they
            // then drag it!

            V2I startP = m_mouse_drag_start_world_pos;
            V2I endP = m_mouse_drag_end_world_pos;

            if (startP.x < endP.x) {
                auto raw_width = max(endP.x - startP.x + 1, m_item_size.x);
                s32 xRemainder = raw_width % m_item_size.x;
                result.set_x(startP.x);
                result.set_width(raw_width - xRemainder);
            } else {
                auto raw_width = startP.x - endP.x + m_item_size.x;
                s32 xRemainder = raw_width % m_item_size.x;
                result.set_x(endP.x + xRemainder);
                result.set_width(raw_width - xRemainder);
            }

            if (startP.y < endP.y) {
                auto raw_height = max(endP.y - startP.y + 1, m_item_size.y);
                s32 yRemainder = raw_height % m_item_size.y;
                result.set_y(startP.y);
                result.set_height(raw_height - yRemainder);
            } else {
                auto raw_height = startP.y - endP.y + m_item_size.y;
                s32 yRemainder = raw_height % m_item_size.y;
                result.set_y(endP.y + yRemainder);
                result.set_height(raw_height - yRemainder);
            }

        } break;

        case DragType::Line: {
            // Axis-aligned straight line, in one dimension.
            // So, if you drag a diagonal line, it picks which direction has greater length and uses that.

            V2I startP = m_mouse_drag_start_world_pos;
            V2I endP = m_mouse_drag_end_world_pos;

            // determine orientation
            s32 xDiff = abs(startP.x - endP.x);
            s32 yDiff = abs(startP.y - endP.y);

            if (xDiff > yDiff) {
                // X
                if (startP.x < endP.x) {
                    auto raw_width = max(xDiff + 1, m_item_size.x);
                    s32 xRemainder = raw_width % m_item_size.x;
                    result.set_x(startP.x);
                    result.set_width(raw_width - xRemainder);
                } else {
                    auto raw_width = xDiff + m_item_size.x;
                    s32 xRemainder = raw_width % m_item_size.x;
                    result.set_x(endP.x + xRemainder);
                    result.set_width(raw_width - xRemainder);
                }

                result.set_y(startP.y);
                result.set_height(m_item_size.y);
            } else {
                // Y
                if (startP.y < endP.y) {
                    auto raw_height = max(yDiff + 1, m_item_size.y);
                    s32 yRemainder = raw_height % m_item_size.y;
                    result.set_y(startP.y);
                    result.set_height(raw_height - yRemainder);
                } else {
                    auto raw_height = yDiff + m_item_size.y;
                    s32 yRemainder = raw_height % m_item_size.y;
                    result.set_y(endP.y + yRemainder);
                    result.set_height(raw_height - yRemainder);
                }

                result.set_x(startP.x);
                result.set_width(m_item_size.x);
            }
        } break;

            INVALID_DEFAULT_CASE;
        }

        result = result.intersected(bounds);
    }

    return result;
}
