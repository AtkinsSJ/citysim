/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Asset.h>
#include <Util/Array.h>
#include <Util/Blob.h>

namespace Assets {

class ContainerAsset final : public Asset {
public:
    ContainerAsset() = default;
    ContainerAsset(Blob data, Array<AssetRef> children);
    virtual ~ContainerAsset() override = default;

    virtual void unload(AssetMetadata&) override;

private:
    Blob m_data;
    Array<AssetRef> m_children;
};

}

using Assets::ContainerAsset;
