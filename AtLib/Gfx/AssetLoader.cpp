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
    BitmapFont::set_asset_type(assets.register_asset_type("BitmapFont"_s, *this, { .directory = "fonts"_sv }));

    m_cursor_defs_type = assets.register_asset_type("CursorDefs"_s, *this, { .file_extension = "cursors"_sv });
    Cursor::set_asset_type(assets.register_asset_type("Cursor"_s, *this, {}));

    m_palette_defs_type = assets.register_asset_type("PaletteDefs"_s, *this, { .file_extension = "palettes"_sv });
    Palette::set_asset_type(assets.register_asset_type("Palette"_s, *this, {}));

    Shader::set_asset_type(assets.register_asset_type("Shader"_s, *this, { .directory = "shaders"_sv }));

    m_sprite_defs_type = assets.register_asset_type("SpriteDefs"_s, *this, { .file_extension = "sprites"_sv });
    SpriteGroup::set_asset_type(assets.register_asset_type("Sprite"_s, *this, {}));
    Ninepatch::set_asset_type(assets.register_asset_type("Ninepatch"_s, *this, {}));

    TextDocument::set_asset_type(assets.register_asset_type("TextDocument"_s, *this, { .file_extension = "txt"_sv }));

    Texture::set_asset_type(assets.register_asset_type("Texture"_s, *this, { .directory = "textures"_sv }));
}

void AssetLoader::create_placeholder_assets(AssetManager& assets)
{
    // NB: Texture needs creating first, as other placeholder assets rely on it.
    assets.set_placeholder_asset(Texture::asset_type(), Texture::make_placeholder());

    assets.set_placeholder_asset(BitmapFont::asset_type(), adopt_own(*new BitmapFont));
    assets.set_placeholder_asset(Cursor::asset_type(), adopt_own(*new Cursor));
    assets.set_placeholder_asset(Ninepatch::asset_type(), Ninepatch::make_placeholder());
    assets.set_placeholder_asset(Palette::asset_type(), adopt_own(*new Palette));
    assets.set_placeholder_asset(Shader::asset_type(), Shader::make_placeholder());
    assets.set_placeholder_asset(SpriteGroup::asset_type(), SpriteGroup::make_placeholder());
    assets.set_placeholder_asset(TextDocument::asset_type(), adopt_own(*new TextDocument));
}

Optional<String> AssetLoader::make_asset_path(AssetManager const& assets, AssetType type, StringView short_name)
{
    if (type == Cursor::asset_type())
        return myprintf("{0}/cursors/{1}"_s, { assets.assetsPath, short_name }, true);
    if (type == BitmapFont::asset_type())
        return myprintf("{0}/fonts/{1}"_s, { assets.assetsPath, short_name }, true);
    if (type == Shader::asset_type())
        return myprintf("{0}/shaders/{1}"_s, { assets.assetsPath, short_name }, true);
    if (type == Texture::asset_type())
        return myprintf("{0}/textures/{1}"_s, { assets.assetsPath, short_name }, true);
    return {};
}

ErrorOr<NonnullOwnPtr<Asset>> AssetLoader::load_asset(AssetMetadata& metadata, Blob file_data)
{
    auto to_error_or_asset = [](auto error_or_asset_subclass) -> ErrorOr<NonnullOwnPtr<Asset>> {
        if (error_or_asset_subclass.is_error())
            return error_or_asset_subclass.release_error();
        return { error_or_asset_subclass.release_value() };
    };

    if (metadata.type == BitmapFont::asset_type())
        return to_error_or_asset(BitmapFont::load_from_bmf_data(metadata, file_data));

    if (metadata.type == m_cursor_defs_type)
        return to_error_or_asset(Cursor::load_defs(metadata, file_data));

    if (metadata.type == m_palette_defs_type)
        return to_error_or_asset(Palette::load_defs(metadata, file_data));

    if (metadata.type == Shader::asset_type())
        return to_error_or_asset(Shader::load(metadata, file_data));

    if (metadata.type == m_sprite_defs_type)
        return to_error_or_asset(load_sprite_defs(metadata, file_data));

    if (metadata.type == TextDocument::asset_type())
        return to_error_or_asset(TextDocument::load(metadata, file_data));

    if (metadata.type == Texture::asset_type())
        return to_error_or_asset(Texture::load(metadata, file_data));

    VERIFY_NOT_REACHED();
}

}
