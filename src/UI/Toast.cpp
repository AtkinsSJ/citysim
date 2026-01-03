/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Toast.h"
#include "AppState.h"
#include <UI/Panel.h>
#include <UI/UI.h>
#include <Util/Interpolate.h>

namespace UI {

Toast::Toast(StringView message, float duration_seconds)
    : m_duration_seconds(duration_seconds)
{
    m_buffer.append(message);
}

void Toast::show(StringView message)
{
    uiState.toasts.push(message, TOAST_APPEAR_TIME + TOAST_DISPLAY_TIME + TOAST_DISAPPEAR_TIME);
}

bool Toast::update_and_draw(float delta_time)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    m_current_time_seconds += delta_time;

    if (m_current_time_seconds >= m_duration_seconds)
        return true;

    PanelStyle* style = getStyle<PanelStyle>("toast"_s);
    V2I origin = v2i(windowSize.x / 2, windowSize.y - 8);
    auto text = m_buffer.view();

    LabelStyle* labelStyle = getStyle<LabelStyle>(style->labelStyle);
    s32 maxWidth = min(floor_s32(windowSize.x * 0.8f), 500);
    V2I textSize = getFont(labelStyle->font).calculate_text_size(text, maxWidth - (style->padding.left + style->padding.right));

    V2I toastSize = v2i(
        textSize.x + (style->padding.left + style->padding.right),
        textSize.y + (style->padding.top + style->padding.bottom));

    float animationDistance = toastSize.y + 16.0f;

    if (m_current_time_seconds < TOAST_APPEAR_TIME) {
        // Animate in
        float t = m_current_time_seconds / TOAST_APPEAR_TIME;
        origin.y += round_s32(interpolate(animationDistance, 0, t, Interpolation::SineOut));
    } else if (m_current_time_seconds > (TOAST_APPEAR_TIME + TOAST_DISPLAY_TIME)) {
        // Animate out
        float t = (m_current_time_seconds - (TOAST_APPEAR_TIME + TOAST_DISPLAY_TIME)) / TOAST_DISAPPEAR_TIME;
        origin.y += round_s32(interpolate(0, animationDistance, t, Interpolation::SineIn));
    }
    Rect2I toastBounds = Rect2I::create_aligned(origin, toastSize, { HAlign::Centre, VAlign::Bottom });

    Panel panel = Panel(toastBounds, style);
    panel.addLabel(text);
    panel.end();
    return false;
}

}
