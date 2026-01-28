/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AssetRef.h"
#include <Assets/AssetManager.h>

AssetMetadata& AssetRef::get() const
{
    auto& assets = asset_manager();
    if (!m_pointer || assets.asset_generation() > m_asset_generation) {
        m_pointer = &getAsset(m_type, m_name);
        m_asset_generation = assets.asset_generation();
    }

    return *m_pointer;
}
