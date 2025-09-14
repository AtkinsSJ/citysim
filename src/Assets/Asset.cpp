/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Asset.h"
#include <Assets/AssetManager.h>
#include <Gfx/SDL.h>
#include <Settings/Settings.h>

Asset::~Asset() = default;

Asset* Asset::make_placeholder(AssetType type)
{
    Asset* asset;

    switch (type) {
    case AssetType::Cursor:
        asset = Cursor::make_placeholder();
        break;
    default:
        asset = asset_manager().arena.allocate<Asset>(type, String {}, String {}, AssetFlags {}, Optional<Locale> {});
        break;
    }

    asset->state = State::Loaded;
    return asset;
}

AssetRef Asset::get_ref() const
{
    return AssetRef { type, shortName };
}

void Asset::load()
{
    DEBUG_FUNCTION();
    if (state != State::Unloaded)
        return;

    if (locale.has_value()) {
        // Only load assets that match our locale
        auto current_locale = get_locale();
        if (locale == current_locale) {
            texts.isFallbackLocale = false;
        } else if (locale == Locale::En) {
            logInfo("Loading asset {0} as a default-locale fallback. (Locale {1}, current is {2})"_s, { fullName, to_string(locale.value()), to_string(current_locale) });
            texts.isFallbackLocale = true;
        } else {
            logInfo("Skipping asset {0} because it's the wrong locale. ({1}, current is {2})"_s, { fullName, to_string(locale.value()), to_string(current_locale) });
            return;
        }
    }

    on_load();
    state = State::Loaded;
}

Cursor* Cursor::make_placeholder()
{
    return asset_manager().arena.allocate<Cursor>(String {}, String {}, AssetFlags {}, String {}, V2I {}, Optional<Locale> {});
}

Cursor::Cursor(String short_name, String full_path, Flags<AssetFlags> flags, String image_file_path, V2I hotspot, Optional<Locale> locale)
    : Asset(AssetType::Cursor, short_name, full_path, flags, move(locale))
    , m_image_file_path(image_file_path)
    , m_hotspot(hotspot)
{
}

void Cursor::on_load()
{
    auto file_data = readTempFile(m_image_file_path);
    SDL_Surface* cursor_surface = create_surface_from_file_data(file_data, shortName);
    m_sdl_cursor = SDL_CreateColorCursor(cursor_surface, m_hotspot.x, m_hotspot.y);
    SDL_FreeSurface(cursor_surface);
}

void Cursor::on_unload()
{
    if (m_sdl_cursor != nullptr) {
        SDL_FreeCursor(m_sdl_cursor);
        m_sdl_cursor = nullptr;
    }
}
