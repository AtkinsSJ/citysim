/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AssetLoader.h"
#include <Assets/AssetManager.h>
#include <Gfx/BitmapFont.h>
#include <Gfx/Cursor.h>
#include <Gfx/Ninepatch.h>
#include <Gfx/Palette.h>
#include <Gfx/Shader.h>
#include <Gfx/Sprite.h>
#include <Gfx/TextDocument.h>
#include <Gfx/Texture.h>
#include <Util/Blob.h>
#include <Util/ErrorOr.h>

namespace Gfx {

void AssetLoader::register_types(AssetManager& assets)
{
    assets.directoryNameToType.put(assets.assetStrings.intern("fonts"_s), AssetType::BitmapFont);
    assets.asset_loaders_by_type[AssetType::BitmapFont] = this;
    BitmapFont::set_asset_type(AssetType::BitmapFont);

    assets.fileExtensionToType.put(assets.assetStrings.intern("cursors"_s), AssetType::CursorDefs);
    assets.asset_loaders_by_type[AssetType::Cursor] = this;
    assets.asset_loaders_by_type[AssetType::CursorDefs] = this;
    Cursor::set_asset_type(AssetType::Cursor);

    assets.fileExtensionToType.put(assets.assetStrings.intern("palettes"_s), AssetType::PaletteDefs);
    assets.asset_loaders_by_type[AssetType::PaletteDefs] = this;
    assets.asset_loaders_by_type[AssetType::Palette] = this;
    Palette::set_asset_type(AssetType::Palette);

    assets.directoryNameToType.put(assets.assetStrings.intern("shaders"_s), AssetType::Shader);
    assets.asset_loaders_by_type[AssetType::Shader] = this;
    Shader::set_asset_type(AssetType::Shader);

    assets.fileExtensionToType.put(assets.assetStrings.intern("sprites"_s), AssetType::SpriteDefs);
    assets.asset_loaders_by_type[AssetType::SpriteDefs] = this;
    assets.asset_loaders_by_type[AssetType::Sprite] = this;
    assets.asset_loaders_by_type[AssetType::Ninepatch] = this;
    SpriteGroup::set_asset_type(AssetType::Sprite);
    Ninepatch::set_asset_type(AssetType::Ninepatch);

    assets.fileExtensionToType.put(assets.assetStrings.intern("txt"_s), AssetType::TextDocument);
    assets.asset_loaders_by_type[AssetType::TextDocument] = this;
    TextDocument::set_asset_type(AssetType::TextDocument);

    assets.directoryNameToType.put(assets.assetStrings.intern("textures"_s), AssetType::Texture);
    assets.asset_loaders_by_type[AssetType::Texture] = this;
    Texture::set_asset_type(AssetType::Texture);
}

void AssetLoader::create_placeholder_assets(AssetManager& assets)
{
    // NB: Texture needs creating first, as other placeholder assets rely on it.
    assets.set_placeholder_asset(AssetType::Texture, Texture::make_placeholder());

    assets.set_placeholder_asset(AssetType::BitmapFont, adopt_own(*new BitmapFont));
    assets.set_placeholder_asset(AssetType::Cursor, adopt_own(*new Cursor));
    assets.set_placeholder_asset(AssetType::Ninepatch, Ninepatch::make_placeholder());
    assets.set_placeholder_asset(AssetType::Palette, adopt_own(*new Palette));
    assets.set_placeholder_asset(AssetType::Shader, Shader::make_placeholder());
    assets.set_placeholder_asset(AssetType::Sprite, SpriteGroup::make_placeholder());
    assets.set_placeholder_asset(AssetType::TextDocument, adopt_own(*new TextDocument));
}

Optional<String> AssetLoader::make_asset_path(AssetManager const& assets, AssetType type, StringView short_name)
{
    switch (type) {
    case AssetType::Cursor:
        return myprintf("{0}/cursors/{1}"_s, { assets.assetsPath, short_name }, true);
    case AssetType::BitmapFont:
        return myprintf("{0}/fonts/{1}"_s, { assets.assetsPath, short_name }, true);
    case AssetType::Shader:
        return myprintf("{0}/shaders/{1}"_s, { assets.assetsPath, short_name }, true);
    case AssetType::Texture:
        return myprintf("{0}/textures/{1}"_s, { assets.assetsPath, short_name }, true);
    default:
        return {};
    }
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

    if (metadata.type == AssetType::CursorDefs)
        return to_error_or_asset(Cursor::load_defs(metadata, file_data));

    if (metadata.type == AssetType::PaletteDefs)
        return to_error_or_asset(Palette::load_defs(metadata, file_data));

    if (metadata.type == AssetType::Shader)
        return to_error_or_asset(Shader::load(metadata, file_data));

    if (metadata.type == AssetType::SpriteDefs)
        return to_error_or_asset(load_sprite_defs(metadata, file_data));

    if (metadata.type == AssetType::TextDocument)
        return to_error_or_asset(TextDocument::load(metadata, file_data));

    if (metadata.type == AssetType::Texture)
        return to_error_or_asset(Texture::load(metadata, file_data));

    VERIFY_NOT_REACHED();
}

}
