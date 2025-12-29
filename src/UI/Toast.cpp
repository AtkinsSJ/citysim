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

void pushToast(StringView message)
{
    Toast* newToast = &uiState.toasts.push();

    *newToast = {};
    // FIXME: StringBuffer
    newToast->text = String { newToast->_chars, MAX_TOAST_LENGTH };
    copyString(message.raw_pointer_to_characters(), message.length(), &newToast->text);

    newToast->duration = TOAST_APPEAR_TIME + TOAST_DISPLAY_TIME + TOAST_DISAPPEAR_TIME;
    newToast->time = 0;
}

void drawToast()
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::UI);

    auto current_toast = uiState.toasts.peek();
    if (current_toast.has_value()) {
        Toast* toast = current_toast.value();

        toast->time += AppState::the().deltaTime;

        if (toast->time >= toast->duration) {
            (void)uiState.toasts.pop();
        } else {
            PanelStyle* style = getStyle<PanelStyle>("toast"_s);
            V2I origin = v2i(windowSize.x / 2, windowSize.y - 8);

            LabelStyle* labelStyle = getStyle<LabelStyle>(style->labelStyle);
            s32 maxWidth = min(floor_s32(windowSize.x * 0.8f), 500);
            V2I textSize = calculateTextSize(getFont(labelStyle->font), toast->text, maxWidth - (style->padding.left + style->padding.right));

            V2I toastSize = v2i(
                textSize.x + (style->padding.left + style->padding.right),
                textSize.y + (style->padding.top + style->padding.bottom));

            float animationDistance = toastSize.y + 16.0f;

            if (toast->time < TOAST_APPEAR_TIME) {
                // Animate in
                float t = toast->time / TOAST_APPEAR_TIME;
                origin.y += round_s32(interpolate(animationDistance, 0, t, Interpolation::SineOut));
            } else if (toast->time > (TOAST_APPEAR_TIME + TOAST_DISPLAY_TIME)) {
                // Animate out
                float t = (toast->time - (TOAST_APPEAR_TIME + TOAST_DISPLAY_TIME)) / TOAST_DISAPPEAR_TIME;
                origin.y += round_s32(interpolate(0, animationDistance, t, Interpolation::SineIn));
            }
            Rect2I toastBounds = Rect2I::create_aligned(origin, toastSize, { HAlign::Centre, VAlign::Bottom });

            Panel panel = Panel(toastBounds, style);
            panel.addLabel(toast->text);
            panel.end();
        }
    }
}

}
