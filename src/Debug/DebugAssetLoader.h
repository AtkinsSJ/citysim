/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetLoader.h>

class DebugAssetLoader final : public Assets::AssetLoader {
public:
    virtual ~DebugAssetLoader() override = default;

    void register_types(AssetManager&) override;
    void create_placeholder_assets(AssetManager&) override;
    ErrorOr<NonnullOwnPtr<Asset>> load_asset(AssetMetadata& metadata, Blob file_data) override;
};
