/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "ContainerAsset.h"
#include <Assets/AssetManager.h>

namespace Assets {

ContainerAsset::ContainerAsset(Blob data, Array<AssetRef> children)
    : m_data(move(data))
    , m_children(move(children))
{
}

void ContainerAsset::unload(AssetMetadata&)
{
    for (auto const& child : m_children)
        removeAsset(child);
    assets_deallocate(m_data);
    m_children = {};
}

}
