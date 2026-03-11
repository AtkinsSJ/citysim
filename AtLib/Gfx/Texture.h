/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <SDL2/SDL_surface.h>
#include <Util/Blob.h>
#include <Util/OwnPtr.h>

class Texture final : public Asset {
    ASSET_SUBCLASS_METHODS(Texture);

public:
    static NonnullOwnPtr<Texture> make_placeholder();
    static ErrorOr<NonnullOwnPtr<Texture>> load(AssetMetadata&, Blob file_data);
    virtual ~Texture() override = default;

    virtual void unload(AssetMetadata& metadata) override;

    SDL_Surface* surface;

    // Renderer-specific stuff.
    struct {
        u32 glTextureID { 0 };
        bool isLoaded { false };
    } gl;

private:
    explicit Texture(SDL_Surface*, Blob pixel_data = {});
    Blob m_pixel_data;
};
