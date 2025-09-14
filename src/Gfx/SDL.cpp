/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "SDL.h"

#include <SDL_image.h>
#include <Util/Blob.h>
#include <Util/Deferred.h>
#include <Util/Log.h>
#include <Util/Maths.h>
#include <Util/String.h>

SDL_Surface* create_surface_from_file_data(Blob bytes, String file_name)
{
    ASSERT(bytes.size() > 0);      //, "Attempted to create a surface from an unloaded asset! ({0})", {name});
    ASSERT(bytes.size() < s32Max); //, "File '{0}' is too big for SDL's RWOps!", {name});

    SDL_RWops* rw = SDL_RWFromConstMem(bytes.data(), truncate32(bytes.size()));
    Deferred deferred = [&] {
        if (rw)
            SDL_RWclose(rw);
    };

    if (!rw)
        logError("Failed to create SDL_RWops from asset '{0}'!\n{1}"_s, { file_name, String::from_null_terminated(SDL_GetError()) });

    if (auto surface = IMG_Load_RW(rw, 0))
        return surface;

    logError("Failed to create SDL_Surface from asset '{0}'!\n{1}"_s, { file_name, String::from_null_terminated(IMG_GetError()) });
    return nullptr;
}
