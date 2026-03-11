/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Texture.h"
#include <Assets/AssetManager.h>
#include <SDL_image.h>

NonnullOwnPtr<Texture> Texture::make_placeholder()
{
    auto pixel_data = Assets::assets_allocate(2 * 2 * sizeof(u32));
    u32* pixels = (u32*)pixel_data.writable_data();
    pixels[0] = pixels[3] = 0xffff00ff;
    pixels[1] = pixels[2] = 0xff000000;
    auto* surface = SDL_CreateRGBSurfaceFrom(pixels, 2, 2, 32, 2 * sizeof(u32),
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

    return adopt_own(*new Texture(surface));
}

// FIXME: Duplicated in the Cursor loading code. Probably we want a single Image asset which Cursor and Texture both refer to.
static ErrorOr<SDL_Surface*> createSurfaceFromFileData(Blob fileData, String name)
{
    SDL_Surface* result = nullptr;

    ASSERT(fileData.size() > 0);      //, "Attempted to create a surface from an unloaded asset! ({0})", {name});
    ASSERT(fileData.size() < s32Max); //, "File '{0}' is too big for SDL's RWOps!", {name});

    SDL_RWops* rw = SDL_RWFromConstMem(fileData.data(), truncate32(fileData.size()));
    if (rw) {
        result = IMG_Load_RW(rw, 0);

        if (result == nullptr) {
            return Error { myprintf("Failed to create SDL_Surface from asset '{0}'!\n{1}"_s, { name, String::from_null_terminated(IMG_GetError()) }) };
        }

        SDL_RWclose(rw);
    } else {
        return Error { myprintf("Failed to create SDL_RWops from asset '{0}'!\n{1}"_s, { name, String::from_null_terminated(SDL_GetError()) }) };
    }

    return result;
}

ErrorOr<NonnullOwnPtr<Texture>> Texture::load(AssetMetadata& metadata, Blob file_data)
{
    auto surface_or_error = createSurfaceFromFileData(file_data, metadata.fullName);
    if (surface_or_error.is_error())
        return surface_or_error.release_error();

    auto* surface = surface_or_error.release_value();

    if (surface->format->BytesPerPixel != 4) {
        auto error = Error { myprintf("Texture asset '{0}' is not 32bit, which is all we support right now. (BytesPerPixel = {1})"_s, { metadata.shortName, formatInt(surface->format->BytesPerPixel) }) };
        SDL_FreeSurface(surface);
        return error;
    }

    // Premultiply alpha
    // NOTE: We always assume the data isn't premultiplied.
    u32 Rmask = surface->format->Rmask,
        Gmask = surface->format->Gmask,
        Bmask = surface->format->Bmask,
        Amask = surface->format->Amask;
    float rRmask = (float)Rmask,
          rGmask = (float)Gmask,
          rBmask = (float)Bmask,
          rAmask = (float)Amask;

    u32 pixelCount = surface->w * surface->h;
    for (u32 pixelIndex = 0;
        pixelIndex < pixelCount;
        pixelIndex++) {
        u32 pixel = ((u32*)surface->pixels)[pixelIndex];
        float rr = (float)(pixel & Rmask) / rRmask;
        float rg = (float)(pixel & Gmask) / rGmask;
        float rb = (float)(pixel & Bmask) / rBmask;
        float ra = (float)(pixel & Amask) / rAmask;

        u32 r = (u32)(rr * ra * rRmask) & Rmask;
        u32 g = (u32)(rg * ra * rGmask) & Gmask;
        u32 b = (u32)(rb * ra * rBmask) & Bmask;
        u32 a = (u32)(ra * rAmask) & Amask;

        ((u32*)surface->pixels)[pixelIndex] = r | g | b | a;
    }

    return adopt_own(*new Texture(surface));
}

Texture::Texture(SDL_Surface* surface, Blob pixel_data)
    : surface(surface)
    , m_pixel_data(move(pixel_data))
{
}

void Texture::unload(AssetMetadata&)
{
    if (surface != nullptr) {
        SDL_FreeSurface(surface);
        surface = nullptr;
    }
    Assets::assets_deallocate(m_pixel_data);
}
