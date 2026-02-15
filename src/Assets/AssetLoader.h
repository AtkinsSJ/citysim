/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/Forward.h>
#include <Util/ErrorOr.h>
#include <Util/OwnPtr.h>

namespace Assets {

class AssetLoader {
public:
    virtual ~AssetLoader() = default;

    virtual void register_types(AssetManager&) = 0;
    virtual void create_placeholder_assets(AssetManager&) = 0;

    virtual ErrorOr<NonnullOwnPtr<Asset>> load_asset(AssetMetadata& metadata, Blob file_data) = 0;
};

}

using Assets::AssetLoader;
