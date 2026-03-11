/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/StringBuffer.h>

namespace UI {

class Toast {
public:
    static void show(StringView message);
    Toast(StringView message, float duration_seconds);

    // Returns true if the toast has completed.
    bool update_and_draw(float delta_time);

private:
    static float constexpr TOAST_APPEAR_TIME = 0.2f;
    static float constexpr TOAST_DISPLAY_TIME = 2.0f;
    static float constexpr TOAST_DISAPPEAR_TIME = 0.2f;
    static s32 constexpr MAX_TOAST_LENGTH = 1024;

    float m_duration_seconds { 0 };
    float m_current_time_seconds { 0 };

    StringBuffer<MAX_TOAST_LENGTH> m_buffer;
};

}
