/*
 * Copyright (c) 2025, Sam Atkins <sam@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AppState.h"

AppState& AppState::the()
{
    static AppState s_app_state;
    return s_app_state;
}
