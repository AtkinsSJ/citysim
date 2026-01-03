/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Sprite.h"
#include <Assets/AssetManager.h>

Sprite& SpriteRef::get() const
{
    if (asset_manager().asset_generation() > m_asset_generation) {
        SpriteGroup* group = getSpriteGroup(m_sprite_group_name.deprecated_to_string());
        if (group != nullptr) {
            m_pointer = group->sprites + (m_sprite_index % group->count);
        } else {
            // TODO: Dummy sprite!
            ASSERT(!"Sprite group missing");
        }

        m_asset_generation = asset_manager().asset_generation();
    }

    return *m_pointer;
}
