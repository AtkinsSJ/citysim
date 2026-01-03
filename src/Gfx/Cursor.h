/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <SDL2/SDL_mouse.h>
#include <Util/String.h>
#include <Util/Vector.h>

struct Cursor {
    String imageFilePath; // Full path
    V2I hotspot;
    SDL_Cursor* sdlCursor;
};
