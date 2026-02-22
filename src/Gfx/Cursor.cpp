/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Cursor.h"
#include <Assets/AssetManager.h>
#include <Assets/AssetRef.h>
#include <Assets/ContainerAsset.h>
#include <SDL_image.h>

// FIXME: Put this somewhere proper eventually. It's currently duplicated with the Texture loading code.
static SDL_Surface* createSurfaceFromFileData(Blob fileData, String name)
{
    SDL_Surface* result = nullptr;

    ASSERT(fileData.size() > 0);      //, "Attempted to create a surface from an unloaded asset! ({0})", {name});
    ASSERT(fileData.size() < s32Max); //, "File '{0}' is too big for SDL's RWOps!", {name});

    SDL_RWops* rw = SDL_RWFromConstMem(fileData.data(), truncate32(fileData.size()));
    if (rw) {
        result = IMG_Load_RW(rw, 0);

        if (result == nullptr) {
            logError("Failed to create SDL_Surface from asset '{0}'!\n{1}"_s, { name, String::from_null_terminated(IMG_GetError()) });
        }

        SDL_RWclose(rw);
    } else {
        logError("Failed to create SDL_RWops from asset '{0}'!\n{1}"_s, { name, String::from_null_terminated(SDL_GetError()) });
    }

    return result;
}

ErrorOr<NonnullOwnPtr<Asset>> Cursor::load_defs(AssetMetadata& metadata, Blob data)
{
    LineReader reader { metadata.shortName, data };

    struct CursorDef {
        StringView name;
        StringView filename;
        V2I hotspot;
    };
    ChunkedArray<CursorDef> cursor_defs;
    initChunkedArray(&cursor_defs, &temp_arena(), 128);

    while (reader.load_next_line()) {
        auto name_token = reader.next_token();
        if (!name_token.has_value())
            continue;

        auto filename = reader.next_token();
        auto hot_x = reader.read_int<s32>();
        auto hot_y = reader.read_int<s32>();
        if (!filename.has_value() || !hot_x.has_value() || !hot_y.has_value())
            return reader.make_error_message("Couldn't parse cursor definition. Expected 'name filename.png hot-x hot-y'."_s);

        cursor_defs.append({
            .name = name_token.release_value(),
            .filename = filename.release_value(),
            .hotspot = v2i(hot_x.release_value(), hot_y.release_value()),
        });
    }

    auto children_data = Assets::assets_allocate(cursor_defs.count * sizeof(GenericAssetRef));
    auto children = makeArray(cursor_defs.count, reinterpret_cast<GenericAssetRef*>(children_data.writable_data()));

    for (auto it = cursor_defs.iterate(); it.hasNext(); it.next()) {
        auto& cursor_def = it.get();

        AssetMetadata* cursor_metadata = asset_manager().add_asset(asset_type(), cursor_def.name, {});

        auto image_path = asset_manager().make_asset_path(asset_type(), cursor_def.filename);
        auto image_data = readTempFile(image_path);
        auto* cursor_surface = createSurfaceFromFileData(image_data, metadata.shortName);
        auto* sdl_cursor = SDL_CreateColorCursor(cursor_surface, cursor_def.hotspot.x, cursor_def.hotspot.y);
        SDL_FreeSurface(cursor_surface);

        cursor_metadata->loaded_asset = adopt_own(*new Cursor(sdl_cursor));
        cursor_metadata->state = AssetMetadata::State::Loaded;
        children.append(cursor_metadata->get_ref());
    }

    return { adopt_own(*new ContainerAsset(move(children_data), move(children))) };
}

Cursor::Cursor()
    : m_sdl_cursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW))
{
}

Cursor::Cursor(SDL_Cursor* sdl_cursor)
    : m_sdl_cursor(sdl_cursor)
{
}

void Cursor::set_as_cursor() const
{
    SDL_SetCursor(m_sdl_cursor);
}

void Cursor::unload(AssetMetadata&)
{
    if (m_sdl_cursor != nullptr) {
        SDL_FreeCursor(m_sdl_cursor);
        m_sdl_cursor = nullptr;
    }
}
