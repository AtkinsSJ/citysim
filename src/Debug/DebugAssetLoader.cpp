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
    Keymap::set_asset_type(assets.register_asset_type("Keymap"_s, *this, { .file_extension = "keymap"_sv }));
}

void DebugAssetLoader::create_placeholder_assets(AssetManager& assets)
{
    assets.set_placeholder_asset(Keymap::asset_type(), adopt_own(*new Keymap));
}

ErrorOr<NonnullOwnPtr<Asset>> DebugAssetLoader::load_asset(AssetMetadata& metadata, Blob file_data)
{
    auto to_error_or_asset = [](auto error_or_asset_subclass) -> ErrorOr<NonnullOwnPtr<Asset>> {
        if (error_or_asset_subclass.is_error())
            return error_or_asset_subclass.release_error();
        return { error_or_asset_subclass.release_value() };
    };

    if (metadata.type == Keymap::asset_type())
        return to_error_or_asset(Keymap::load(metadata, file_data));

    VERIFY_NOT_REACHED();
}
