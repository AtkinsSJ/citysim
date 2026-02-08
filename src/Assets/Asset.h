/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>

namespace Assets {

class Asset {
public:
    virtual ~Asset() = default;

    virtual void unload(AssetMetadata& metadata) = 0;
};

// FIXME: Move child-asset concept into this.
class ContainerAsset final : public Asset {
public:
    virtual ~ContainerAsset() override = default;
    virtual void unload(AssetMetadata&) override { }
};

}

using Assets::Asset;
using Assets::ContainerAsset;
