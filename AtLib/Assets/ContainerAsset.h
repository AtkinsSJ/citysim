/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Assets/AssetManager.h>
#include <Util/Array.h>
#include <Util/Blob.h>

namespace Assets {

class ContainerAsset final : public Asset {
public:
    ContainerAsset() = default;
    ContainerAsset(Blob data, Array<GenericAssetRef> children)
        : m_data(move(data))
        , m_children(move(children))
    {
    }
    virtual ~ContainerAsset() override = default;

    virtual void unload(AssetMetadata&) override
    {
        for (auto const& child : m_children)
            removeAsset(child);
        assets_deallocate(m_data);
        m_children = {};
    }

private:
    Blob m_data;
    Array<GenericAssetRef> m_children;
};

}

using Assets::ContainerAsset;
