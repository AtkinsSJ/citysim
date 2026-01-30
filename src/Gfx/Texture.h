/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <SDL2/SDL_surface.h>
#include <Util/Basic.h>

struct Texture {
    SDL_Surface* surface;

    // Renderer-specific stuff.
    struct {
        u32 glTextureID;
        bool isLoaded;
    } gl;
};
