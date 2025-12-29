/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/String.h>

namespace UI {

float const TOAST_APPEAR_TIME = 0.2f;
float const TOAST_DISPLAY_TIME = 2.0f;
float const TOAST_DISAPPEAR_TIME = 0.2f;
s32 const MAX_TOAST_LENGTH = 1024;
struct Toast {
    float duration;
    float time; // In seconds, from 0 to duration

    String text;
    char _chars[MAX_TOAST_LENGTH];
};

// NB: `message` is copied into the UIState, so it can be a temporary allocation
void pushToast(StringView message);
void drawToast();

}
