/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AssetLoader.h"
#include <Assets/AssetManager.h>
#include <Assets/AssetMetadata.h>
#include <Assets/ContainerAsset.h>
#include <UI/UITheme.h>

namespace UI {

void AssetLoader::register_types(AssetManager& assets)
{
    assets.fileExtensionToType.put(assets.assetStrings.intern("theme"_s), AssetType::UITheme);
    assets.asset_loaders_by_type[AssetType::UITheme] = this;
}

void AssetLoader::create_placeholder_assets(AssetManager& assets)
{
    assets.set_placeholder_asset(AssetType::UITheme, adopt_own(*new ContainerAsset));
}

ErrorOr<NonnullOwnPtr<Asset>> AssetLoader::load_asset(AssetMetadata& metadata, Blob file_data)
{
    if (metadata.type == AssetType::UITheme)
        return load_theme(metadata, file_data);

    VERIFY_NOT_REACHED();
}

}
