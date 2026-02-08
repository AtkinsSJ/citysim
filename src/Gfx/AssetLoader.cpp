/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AssetLoader.h"
#include <Assets/AssetManager.h>
#include <Gfx/BitmapFont.h>
#include <Gfx/Palette.h>
#include <Gfx/TextDocument.h>
#include <Util/Blob.h>
#include <Util/ErrorOr.h>

namespace Gfx {

void AssetLoader::register_types(AssetManager& assets)
{
    assets.directoryNameToType.put(assets.assetStrings.intern("fonts"_s), AssetType::BitmapFont);
    assets.asset_loaders_by_type[AssetType::BitmapFont] = this;

    assets.fileExtensionToType.put(assets.assetStrings.intern("palettes"_s), AssetType::PaletteDefs);
    assets.asset_loaders_by_type[AssetType::PaletteDefs] = this;
    assets.asset_loaders_by_type[AssetType::Palette] = this;

    assets.fileExtensionToType.put(assets.assetStrings.intern("txt"_s), AssetType::TextDocument);
    assets.asset_loaders_by_type[AssetType::TextDocument] = this;
}

void AssetLoader::create_placeholder_assets(AssetManager& assets)
{
    assets.set_placeholder_asset(AssetType::BitmapFont, adopt_own(*new BitmapFont));
    assets.set_placeholder_asset(AssetType::Palette, adopt_own(*new Palette));
    assets.set_placeholder_asset(AssetType::TextDocument, adopt_own(*new TextDocument));
}

ErrorOr<NonnullOwnPtr<Asset>> AssetLoader::load_asset(AssetMetadata& metadata, Blob file_data)
{
    auto to_error_or_asset = [](auto error_or_asset_subclass) -> ErrorOr<NonnullOwnPtr<Asset>> {
        if (error_or_asset_subclass.is_error())
            return error_or_asset_subclass.release_error();
        return { error_or_asset_subclass.release_value() };
    };

    if (metadata.type == AssetType::BitmapFont)
        return to_error_or_asset(BitmapFont::load_from_bmf_data(metadata, file_data));

    if (metadata.type == AssetType::PaletteDefs)
        return to_error_or_asset(Palette::load_defs(metadata, file_data));

    if (metadata.type == AssetType::TextDocument)
        return to_error_or_asset(TextDocument::load(metadata, file_data));

    VERIFY_NOT_REACHED();
}

}
