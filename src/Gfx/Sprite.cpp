/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Sprite.h"
#include <Assets/AssetManager.h>

Sprite& Sprite::get(StringView name)
{
    return SpriteGroup::get(name).sprites[0];
}

SpriteGroup& SpriteGroup::get(StringView name)
{
    return getAsset(AssetType::Sprite, name.deprecated_to_string()).spriteGroup;
}

Sprite& SpriteRef::get() const
{
    if (asset_manager().asset_generation() > m_asset_generation) {
        auto& group = SpriteGroup::get(m_sprite_group_name);
        m_pointer = &group.sprites[m_sprite_index % group.count];
        m_asset_generation = asset_manager().asset_generation();
    }

    return *m_pointer;
}
