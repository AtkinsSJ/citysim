/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Ninepatch.h"
#include <Assets/AssetManager.h>
#include <Gfx/Texture.h>
#include <Util/OwnPtr.h>

Ninepatch::Ninepatch(AssetMetadata& texture_metadata, s32 pu0, s32 pu1, s32 pu2, s32 pu3, s32 pv0, s32 pv1, s32 pv2, s32 pv3)
    : texture(&texture_metadata)
    , pu0(pu0)
    , pu1(pu1)
    , pu2(pu2)
    , pu3(pu3)
    , pv0(pv0)
    , pv1(pv1)
    , pv2(pv2)
    , pv3(pv3)
{
    texture_metadata.ensure_is_loaded();

    auto& surface = *dynamic_cast<Texture&>(*texture_metadata.loaded_asset).surface;
    float texture_width = surface.w;
    float texture_height = surface.h;

    u0 = pu0 / texture_width;
    u1 = pu1 / texture_width;
    u2 = pu2 / texture_width;
    u3 = pu3 / texture_width;

    v0 = pv0 / texture_height;
    v1 = pv1 / texture_height;
    v2 = pv2 / texture_height;
    v3 = pv3 / texture_height;
}

NonnullOwnPtr<Ninepatch> Ninepatch::make_placeholder()
{
    auto& placeholder_texture = asset_manager().get_placeholder_asset(Texture::asset_type());
    auto& surface = *dynamic_cast<Texture&>(*placeholder_texture.loaded_asset).surface;
    return adopt_own(*new Ninepatch(placeholder_texture, 0, 0, surface.w, surface.w, 0, 0, surface.h, surface.h));
}

void Ninepatch::unload(AssetMetadata&)
{
    // No-op
}
