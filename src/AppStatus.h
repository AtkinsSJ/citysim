/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

enum class AppStatus : u8 {
    MainMenu,
    Game,
    Credits,
    Quit,
};
