/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DebugAssetLoader.h"
#include <Debug/Keymap.h>
#include <Util/ErrorOr.h>

void DebugAssetLoader::register_types(AssetManager& assets)
{
    assets.fileExtensionToType.put(assets.assetStrings.intern("keymap"_s), AssetType::Keymap);
    assets.asset_loaders_by_type[AssetType::Keymap] = this;
    Keymap::set_asset_type(AssetType::Keymap);
}

void DebugAssetLoader::create_placeholder_assets(AssetManager& assets)
{
    assets.set_placeholder_asset(AssetType::Keymap, adopt_own(*new Keymap));
}

ErrorOr<NonnullOwnPtr<Asset>> DebugAssetLoader::load_asset(AssetMetadata& metadata, Blob file_data)
{
    auto to_error_or_asset = [](auto error_or_asset_subclass) -> ErrorOr<NonnullOwnPtr<Asset>> {
        if (error_or_asset_subclass.is_error())
            return error_or_asset_subclass.release_error();
        return { error_or_asset_subclass.release_value() };
    };

    if (metadata.type == AssetType::Keymap)
        return to_error_or_asset(Keymap::load(metadata, file_data));

    VERIFY_NOT_REACHED();
}
