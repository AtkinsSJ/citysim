/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>
#include <Util/Rectangle.h>
#include <Util/String.h>

struct Sprite {
    static Sprite& get(StringView name);

    Asset* texture;
    Rect2 uv; // in (0 to 1) space
    s32 pixelWidth;
    s32 pixelHeight;
};

struct SpriteGroup {
    static SpriteGroup& get(StringView name);

    s32 count;
    Sprite* sprites;
};

class SpriteRef {
public:
    explicit SpriteRef(StringView group_name, u32 sprite_index)
        : m_sprite_group_name(group_name)
        , m_sprite_index(sprite_index)
    {
    }

    // FIXME: This is only needed because of how we allocate arrays. Fix that, and we can remove this invalid state.
    SpriteRef() = default;

    Sprite& get() const;

private:
    StringView m_sprite_group_name;
    u32 m_sprite_index;

    mutable Sprite* m_pointer { nullptr };
    mutable u32 m_asset_generation { 0 };
};
