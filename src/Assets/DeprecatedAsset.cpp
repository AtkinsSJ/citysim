/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DeprecatedAsset.h"

#include "Util/StringBuilder.h"

#include <Assets/AssetManager.h>
#include <Assets/AssetMetadata.h>
#include <Debug/Console.h>
#include <IO/File.h>
#include <SDL_image.h>
#include <Settings/Settings.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/TerrainCatalogue.h>

DeprecatedAsset::DeprecatedAsset()
    : data()
    , _localData()
{
}

DeprecatedAsset::~DeprecatedAsset() = default;

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

static void copyFileIntoAsset(Blob* fileData, DeprecatedAsset& asset)
{
    asset.data = assetsAllocate(&asset_manager(), fileData->size());
    memcpy(asset.data.writable_data(), fileData->data(), fileData->size());

    // NB: We set the fileData to point at the new copy, so that code after calling copyFileIntoAsset()
    // can still use fileData without having to mess with it. Already had one bug caused by not doing this!
    // FIXME: Stop doing this?
    *fileData = asset.data;
}

static void loadTexts(HashTable<String>* texts, AssetMetadata* metadata, Blob file_data, DeprecatedAsset& asset)
{
    LineReader reader { metadata->shortName, file_data };

    // NB: We store the strings inside the asset data, so it's one block of memory instead of many small ones.
    // However, since we allocate before we parse the file, we need to make sure that the output texts are
    // never longer than the input texts, or we could run out of space!
    // Right now, the only processing we do is replacing \n with a newline character, and similar, so the
    // output can only ever be smaller or the same size as the input.
    // - Sam, 01/10/2019

    // We also put an array of keys into the same allocation.
    // We use the number of lines in the file as a heuristic - we know it'll be slightly more than
    // the number of texts in the file, because they're 1 per line, and we don't have many blanks.

    auto line_count = reader.line_count();
    auto key_array_size = sizeof(String) * line_count;
    asset.data = assetsAllocate(&asset_manager(), file_data.size() + key_array_size);
    asset.texts.keys = makeArray(line_count, reinterpret_cast<String*>(asset.data.writable_data()));

    auto text_data = asset.data.sub_blob(key_array_size);
    StringBuilder string_data_builder { text_data };
    char* write_position = reinterpret_cast<char*>(text_data.writable_data());

    while (reader.load_next_line()) {
        auto input_key = reader.next_token();
        if (!input_key.has_value())
            continue;
        auto input_text = reader.remainder_of_current_line();

        // Store the key
        string_data_builder.append(input_key.value());
        String key { write_position, input_key.value().length() };
        write_position += key.length();

        // Store the text
        auto* text_start = write_position;
        auto text_length = 0u;

        for (s32 charIndex = 0; charIndex < input_text.length(); charIndex++) {
            char c = input_text[charIndex];
            if (c == '\\') {
                if (charIndex + 1 < input_text.length() && input_text[charIndex + 1] == 'n') {
                    string_data_builder.append('\n');
                    text_length++;
                    charIndex++;
                    continue;
                }
            }

            string_data_builder.append(c);
            text_length++;
        }

        write_position += text_length;

        // Check that we don't already have a text with that name.
        // If we do, one will overwrite the other, and that could be unpredictable if they're
        // in different files. Things will still work, but it would be confusing! And unintended.
        auto existing_text = texts->find_value(key);
        if (existing_text.has_value() && existing_text != key) {
            reader.warn("Text asset with ID '{0}' already exists in the texts table! Existing value: \"{1}\""_s, { key, existing_text.value() });
        }

        asset.texts.keys.append(key);

        texts->put(key, String { text_start, text_length });
    }
}

void DeprecatedAsset::unload(AssetMetadata& metadata)
{
    switch (metadata.type) {
    case AssetType::BuildingDefs: {
        // Remove all of our terrain defs
        removeBuildingDefs(buildingDefs.buildingIDs);
        buildingDefs.buildingIDs = makeEmptyArray<String>();
    } break;

    case AssetType::Cursor: {
        if (cursor.sdlCursor != nullptr) {
            SDL_FreeCursor(cursor.sdlCursor);
            cursor.sdlCursor = nullptr;
        }
    } break;

    case AssetType::TerrainDefs: {
        // Remove all of our terrain defs
        removeTerrainDefs(terrainDefs.terrainIDs);
        terrainDefs.terrainIDs = makeEmptyArray<String>();
    } break;

    case AssetType::TextDocument: {
        text_document.unload();
    } break;

    case AssetType::Texts: {
        // Remove all of our texts from the table
        HashTable<String>* textsTable = (texts.isFallbackLocale ? &asset_manager().defaultTexts : &asset_manager().texts);
        for (auto const& key : texts.keys)
            textsTable->removeKey(key);
        texts.keys = makeEmptyArray<String>();
    } break;

    case AssetType::Texture: {
        if (texture.surface != nullptr) {
            SDL_FreeSurface(texture.surface);
            texture.surface = nullptr;
        }
    } break;

    default:
        break;
    }

    if (data.data() != nullptr) {
        asset_manager().assetMemoryAllocated -= data.size();
        deallocateRaw(data.writable_data());
        data = {};
    }
}

static DeprecatedAsset& make_placeholder_asset(AssetType type)
{
    AssetMetadata& metadata = asset_manager().placeholderAssets[type];
    metadata.type = type;
    metadata.shortName = {};
    metadata.fullName = {};
    metadata.flags = {};
    metadata.state = AssetMetadata::State::Loaded;
    metadata.children = makeEmptyArray<AssetRef>();

    metadata.loaded_asset = adopt_own(*new DeprecatedAsset);
    return static_cast<DeprecatedAsset&>(*metadata.loaded_asset);
}

void DeprecatedAssetLoader::register_types(AssetManager& assets)
{
    assets.fileExtensionToType.put(assets.assetStrings.intern("buildings"_s), AssetType::BuildingDefs);
    assets.fileExtensionToType.put(assets.assetStrings.intern("cursors"_s), AssetType::CursorDefs);
    assets.fileExtensionToType.put(assets.assetStrings.intern("keymap"_s), AssetType::DevKeymap);
    assets.fileExtensionToType.put(assets.assetStrings.intern("palettes"_s), AssetType::PaletteDefs);
    assets.fileExtensionToType.put(assets.assetStrings.intern("sprites"_s), AssetType::SpriteDefs);
    assets.fileExtensionToType.put(assets.assetStrings.intern("terrain"_s), AssetType::TerrainDefs);
    assets.fileExtensionToType.put(assets.assetStrings.intern("theme"_s), AssetType::UITheme);
    assets.fileExtensionToType.put(assets.assetStrings.intern("txt"_s), AssetType::TextDocument);

    assets.directoryNameToType.put(assets.assetStrings.intern("fonts"_s), AssetType::BitmapFont);
    assets.directoryNameToType.put(assets.assetStrings.intern("shaders"_s), AssetType::Shader);
    assets.directoryNameToType.put(assets.assetStrings.intern("textures"_s), AssetType::Texture);
    assets.directoryNameToType.put(assets.assetStrings.intern("locale"_s), AssetType::Texts);

    assets.asset_loaders_by_type[AssetType::BitmapFont] = this;
    assets.asset_loaders_by_type[AssetType::BuildingDefs] = this;
    assets.asset_loaders_by_type[AssetType::Cursor] = this;
    assets.asset_loaders_by_type[AssetType::CursorDefs] = this;
    assets.asset_loaders_by_type[AssetType::DevKeymap] = this;
    assets.asset_loaders_by_type[AssetType::Ninepatch] = this;
    assets.asset_loaders_by_type[AssetType::Palette] = this;
    assets.asset_loaders_by_type[AssetType::PaletteDefs] = this;
    assets.asset_loaders_by_type[AssetType::Shader] = this;
    assets.asset_loaders_by_type[AssetType::Sprite] = this;
    assets.asset_loaders_by_type[AssetType::SpriteDefs] = this;
    assets.asset_loaders_by_type[AssetType::TerrainDefs] = this;
    assets.asset_loaders_by_type[AssetType::TextDocument] = this;
    assets.asset_loaders_by_type[AssetType::Texts] = this;
    assets.asset_loaders_by_type[AssetType::Texture] = this;
    assets.asset_loaders_by_type[AssetType::UITheme] = this;
}

void DeprecatedAssetLoader::create_placeholder_assets(AssetManager& assets)
{
    // BitmapFont
    make_placeholder_asset(AssetType::BitmapFont);

    // BuildingDefs
    make_placeholder_asset(AssetType::BuildingDefs);

    // Cursor
    auto& placeholderCursor = make_placeholder_asset(AssetType::Cursor);
    placeholderCursor.cursor.sdlCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);

    // CursorDefs
    make_placeholder_asset(AssetType::CursorDefs);

    // DevKeymap
    make_placeholder_asset(AssetType::DevKeymap);

    // Ninepatch
    auto& placeholderNinepatch = make_placeholder_asset(AssetType::Ninepatch);
    placeholderNinepatch.ninepatch.texture = &assets.placeholderAssets[AssetType::Texture];

    // Palette
    auto& placeholderPalette = make_placeholder_asset(AssetType::Palette);
    placeholderPalette.palette.type = Palette::Type::Fixed;
    placeholderPalette.palette.size = 0;
    placeholderPalette.palette.paletteData = makeEmptyArray<Colour>();

    // PaletteDefs
    make_placeholder_asset(AssetType::PaletteDefs);

    // Shader
    make_placeholder_asset(AssetType::Shader);

    // Sprite!
    auto& placeholderSprite = make_placeholder_asset(AssetType::Sprite);
    placeholderSprite.data = assetsAllocate(&assets, 1 * sizeof(Sprite));
    placeholderSprite.spriteGroup.count = 1;
    placeholderSprite.spriteGroup.sprites = (Sprite*)placeholderSprite.data.writable_data();
    placeholderSprite.spriteGroup.sprites[0].texture = &assets.placeholderAssets[AssetType::Texture];
    placeholderSprite.spriteGroup.sprites[0].uv = { 0.0f, 0.0f, 1.0f, 1.0f };

    // SpriteDefs
    make_placeholder_asset(AssetType::SpriteDefs);

    // TerrainDefs
    make_placeholder_asset(AssetType::TerrainDefs);

    // TextDocument
    make_placeholder_asset(AssetType::TextDocument);

    // Texts
    make_placeholder_asset(AssetType::Texts);

    // Texture
    auto& placeholderTexture = make_placeholder_asset(AssetType::Texture);
    placeholderTexture.data = assetsAllocate(&assets, 2 * 2 * sizeof(u32));
    u32* pixels = (u32*)placeholderTexture.data.writable_data();
    pixels[0] = pixels[3] = 0xffff00ff;
    pixels[1] = pixels[2] = 0xff000000;
    placeholderTexture.texture.surface = SDL_CreateRGBSurfaceFrom(pixels, 2, 2, 32, 2 * sizeof(u32),
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

    // UITheme
    make_placeholder_asset(AssetType::UITheme);
}

static void load_cursor_defs(Blob data, AssetMetadata& metadata, DeprecatedAsset&)
{
    DEBUG_FUNCTION();

    LineReader reader { metadata.shortName, data };

    // We store the cursorNames array in the defs asset
    // So, we first need to scan through the file to see how many cursors there are in it!
    s32 cursorCount = 0;
    while (reader.load_next_line()) {
        cursorCount++;
    }

    allocateChildren(&metadata, cursorCount);

    reader.restart();

    while (reader.load_next_line()) {
        auto name_token = reader.next_token();
        auto filename = reader.next_token();
        if (!name_token.has_value() || !filename.has_value())
            continue;

        String name = asset_manager().assetStrings.intern(name_token.release_value());

        auto hot_x = reader.read_int<s32>();
        auto hot_y = reader.read_int<s32>();

        if (hot_x.has_value() && hot_y.has_value()) {
            // Add the cursor
            AssetMetadata* cursor_metadata = asset_manager().add_asset(AssetType::Cursor, name, {});
            auto cursor_asset = adopt_own(*new DeprecatedAsset);
            cursor_asset->cursor.imageFilePath = asset_manager().assetStrings.intern(asset_manager().make_asset_path(AssetType::Cursor, filename.value()));
            cursor_asset->cursor.hotspot = v2i(hot_x.release_value(), hot_y.release_value());

            auto image_data = readTempFile(cursor_asset->cursor.imageFilePath);
            SDL_Surface* cursorSurface = createSurfaceFromFileData(image_data, metadata.shortName);
            cursor_asset->cursor.sdlCursor = SDL_CreateColorCursor(cursorSurface, cursor_asset->cursor.hotspot.x, cursor_asset->cursor.hotspot.y);
            SDL_FreeSurface(cursorSurface);

            cursor_metadata->loaded_asset = move(cursor_asset);
            addChildAsset(&metadata, cursor_metadata);
        } else {
            // FIXME: Return an Error too.
            reader.error("Couldn't parse cursor definition. Expected 'name filename.png hot-x hot-y'."_s);
            return;
        }
    }
}

static void load_palette_defs(Blob data, AssetMetadata& metadata, DeprecatedAsset&)
{
    DEBUG_FUNCTION();

    LineReader reader { metadata.shortName, data };

    // We store the paletteNames array in the defs asset
    // So, we first need to scan through the file to see how many palettes there are in it!
    s32 paletteCount = 0;
    while (reader.load_next_line()) {
        if (auto command = reader.next_token(); command == ":Palette"_s)
            paletteCount++;
    }

    allocateChildren(&metadata, paletteCount);

    reader.restart();

    AssetMetadata* current_palette_metadata = nullptr;
    bool palette_array_is_initialized = false;
    while (reader.load_next_line()) {
        auto command_token = reader.next_token();
        if (!command_token.has_value())
            continue;
        auto command = command_token.release_value();

        if (command.starts_with(':')) {
            command = command.substring(1);

            if (command == "Palette"_s) {
                if (auto palette_name = reader.next_token(); palette_name.has_value()) {
                    current_palette_metadata = asset_manager().add_asset(AssetType::Palette, palette_name.release_value(), {});
                    current_palette_metadata->loaded_asset = adopt_own(*new DeprecatedAsset);
                    palette_array_is_initialized = false;
                    addChildAsset(&metadata, current_palette_metadata);
                } else {
                    reader.error("Missing name for Palette"_s);
                    return;
                }
            } else {
                reader.error("Unexpected command ':{0}' in palette-definitions file. Only :Palette is allowed!"_s, { command });
                return;
            }
        } else {
            if (current_palette_metadata == nullptr) {
                reader.error("Unexpected command '{0}' before the start of a :Palette"_s, { command });
                return;
            }
            auto& palette_asset = dynamic_cast<DeprecatedAsset&>(*current_palette_metadata->loaded_asset);

            if (command == "type"_s) {
                auto type = reader.next_token();
                if (!type.has_value()) {
                    reader.error("Missing palette type"_s);
                    return;
                }

                if (type == "fixed"_s) {
                    palette_asset.palette.type = Palette::Type::Fixed;
                } else if (type == "gradient"_s) {
                    palette_asset.palette.type = Palette::Type::Gradient;
                } else {
                    reader.error("Unrecognised palette type '{0}', allowed values are: fixed, gradient"_s, { type.value() });
                    return;
                }
            } else if (command == "size"_s) {
                if (auto size = reader.read_int<s32>(); size.has_value()) {
                    palette_asset.palette.size = size.release_value();
                } else {
                    return;
                }
            } else if (command == "color"_s) {
                if (auto color = Colour::read(reader); color.has_value()) {
                    if (palette_asset.palette.type == Palette::Type::Fixed) {
                        if (!palette_array_is_initialized) {
                            palette_asset.data = assetsAllocate(&asset_manager(), palette_asset.palette.size * sizeof(Colour));
                            palette_asset.palette.paletteData = makeArray<Colour>(palette_asset.palette.size, reinterpret_cast<Colour*>(palette_asset.data.writable_data()));
                            palette_array_is_initialized = true;
                        }

                        s32 colorIndex = palette_asset.palette.paletteData.count;
                        if (colorIndex >= palette_asset.palette.size) {
                            reader.error("Too many 'color' definitions! 'size' must be large enough."_s);
                            return;
                        }
                        palette_asset.palette.paletteData.append(color.release_value());
                    } else {
                        reader.error("'color' is only a valid command for fixed palettes."_s);
                        return;
                    }
                }
            } else if (command == "from"_s) {
                if (auto from = Colour::read(reader); from.has_value()) {
                    if (palette_asset.palette.type == Palette::Type::Gradient) {
                        palette_asset.palette.gradient.from = from.release_value();
                    } else {
                        reader.error("'from' is only a valid command for gradient palettes."_s);
                        return;
                    }
                }
            } else if (command == "to"_s) {
                if (auto to = Colour::read(reader); to.has_value()) {
                    if (palette_asset.palette.type == Palette::Type::Gradient) {
                        palette_asset.palette.gradient.to = to.release_value();
                    } else {
                        reader.error("'to' is only a valid command for gradient palettes."_s);
                        return;
                    }
                }
            } else {
                reader.error("Unrecognised command '{0}'"_s, { command });
                return;
            }
        }
    }

    // Load all the palettes, now that we know their properties are all set.
    for (auto& child : metadata.children) {
        auto& palette_metadata = child.get();
        auto& palette_asset = dynamic_cast<DeprecatedAsset&>(*palette_metadata.loaded_asset);
        auto& palette = palette_asset.palette;
        switch (palette.type) {
        case Palette::Type::Gradient: {
            palette_asset.data = assetsAllocate(&asset_manager(), palette.size * sizeof(Colour));
            palette.paletteData = makeArray<Colour>(palette.size, reinterpret_cast<Colour*>(palette_asset.data.writable_data()), palette.size);

            float ratio = 1.0f / (float)(palette.size);
            for (s32 i = 0; i < palette.size; i++) {
                palette.paletteData[i] = lerp(palette.gradient.from, palette.gradient.to, i * ratio);
            }
        } break;

        case Palette::Type::Fixed: {
        } break;
        }
    }
}

static AssetMetadata* add_sprite_group(StringView name, s32 spriteCount)
{
    ASSERT(spriteCount > 0); // Must have a positive number of sprites in a Sprite Group!

    AssetMetadata* metadata = asset_manager().add_asset(AssetType::Sprite, name, {});
    auto asset = adopt_own(*new DeprecatedAsset);
    if (asset->data.size() != 0)
        DEBUG_BREAK(); // @Leak! Creating the sprite group multiple times is probably a bad idea for other reasons too.
    asset->data = assetsAllocate(&asset_manager(), spriteCount * sizeof(Sprite));
    asset->spriteGroup.count = spriteCount;
    asset->spriteGroup.sprites = (Sprite*)asset->data.writable_data();

    metadata->loaded_asset = move(asset);
    return metadata;
}

static AssetMetadata* add_ninepatch(StringView name, StringView filename, s32 pu0, s32 pu1, s32 pu2, s32 pu3, s32 pv0, s32 pv1, s32 pv2, s32 pv3)
{
    AssetMetadata* texture_metadata = asset_manager().add_asset(AssetType::Texture, filename);
    texture_metadata->ensure_is_loaded();

    AssetMetadata* metadata = asset_manager().add_asset(AssetType::Ninepatch, name, {});
    auto asset = adopt_own(*new DeprecatedAsset);

    Ninepatch& ninepatch = asset->ninepatch;
    ninepatch.texture = texture_metadata;

    auto& texture = dynamic_cast<DeprecatedAsset&>(*texture_metadata->loaded_asset).texture;
    float textureWidth = texture.surface->w;
    float textureHeight = texture.surface->h;

    ninepatch.pu0 = pu0;
    ninepatch.pu1 = pu1;
    ninepatch.pu2 = pu2;
    ninepatch.pu3 = pu3;

    ninepatch.pv0 = pv0;
    ninepatch.pv1 = pv1;
    ninepatch.pv2 = pv2;
    ninepatch.pv3 = pv3;

    ninepatch.u0 = ninepatch.pu0 / textureWidth;
    ninepatch.u1 = ninepatch.pu1 / textureWidth;
    ninepatch.u2 = ninepatch.pu2 / textureWidth;
    ninepatch.u3 = ninepatch.pu3 / textureWidth;

    ninepatch.v0 = ninepatch.pv0 / textureHeight;
    ninepatch.v1 = ninepatch.pv1 / textureHeight;
    ninepatch.v2 = ninepatch.pv2 / textureHeight;
    ninepatch.v3 = ninepatch.pv3 / textureHeight;

    metadata->loaded_asset = move(asset);
    return metadata;
}

static void load_sprite_defs(Blob data, AssetMetadata& metadata, DeprecatedAsset&)
{
    DEBUG_FUNCTION();

    LineReader reader { metadata.shortName, data };

    AssetMetadata* textureAsset = nullptr;
    V2I spriteSize = v2i(0, 0);
    V2I spriteBorder = v2i(0, 0);
    AssetMetadata* spriteGroup = nullptr;
    s32 spriteIndex = 0;

    // Count the number of child assets, so we can allocate our spriteNames array
    s32 childAssetCount = 0;
    while (reader.load_next_line()) {
        if (auto command = reader.next_token(); command.has_value() && command.value().starts_with(':'))
            childAssetCount++;
    }
    allocateChildren(&metadata, childAssetCount);

    reader.restart();

    // Now, actually read things
    while (reader.load_next_line()) {
        auto maybe_command = reader.next_token();
        if (!maybe_command.has_value())
            continue;
        auto command = maybe_command.release_value();

        if (command.starts_with(':')) // Definitions
        {
            // Define something
            command = command.substring(1).deprecated_to_string();

            textureAsset = nullptr;
            spriteGroup = nullptr;

            if (command == "Ninepatch"_s) {
                auto name = reader.next_token();
                auto filename = reader.next_token();
                auto pu0 = reader.read_int<s32>();
                auto pu1 = reader.read_int<s32>();
                auto pu2 = reader.read_int<s32>();
                auto pu3 = reader.read_int<s32>();
                auto pv0 = reader.read_int<s32>();
                auto pv1 = reader.read_int<s32>();
                auto pv2 = reader.read_int<s32>();
                auto pv3 = reader.read_int<s32>();

                if (!all_have_values(name, filename, pu0, pu1, pu2, pu3, pv0, pv1, pv2, pv3)) {
                    reader.error("Couldn't parse Ninepatch. Expected: ':Ninepatch identifier filename.png pu0 pu1 pu2 pu3 pv0 pv1 pv2 pv3'"_s);
                    return;
                }

                AssetMetadata* ninepatch = add_ninepatch(name.release_value(), filename.release_value(), pu0.release_value(), pu1.release_value(), pu2.release_value(), pu3.release_value(), pv0.release_value(), pv1.release_value(), pv2.release_value(), pv3.release_value());

                addChildAsset(&metadata, ninepatch);
            } else if (command == "Sprite"_s) {
                // @Copypasta from the SpriteGroup branch, and the 'sprite' property
                auto name = reader.next_token();
                auto filename = reader.next_token();
                auto spriteSizeIn = V2I::read(reader);

                if (!all_have_values(name, filename, spriteSizeIn)) {
                    reader.error("Couldn't parse Sprite. Expected: ':Sprite identifier filename.png SWxSH'"_s);
                    return;
                }

                spriteSize = spriteSizeIn.release_value();

                AssetMetadata* group = add_sprite_group(name.release_value(), 1);
                auto& group_asset = dynamic_cast<DeprecatedAsset&>(*group->loaded_asset);

                Sprite* sprite = group_asset.spriteGroup.sprites;
                sprite->texture = asset_manager().add_asset(AssetType::Texture, filename.release_value());
                sprite->uv = { 0, 0, spriteSize.x, spriteSize.y };
                sprite->pixelWidth = spriteSize.x;
                sprite->pixelHeight = spriteSize.y;

                addChildAsset(&metadata, group);
            } else if (command == "SpriteGroup"_s) {
                auto name = reader.next_token();
                auto filename = reader.next_token();
                auto spriteSizeIn = V2I::read(reader);

                if (!all_have_values(name, filename, spriteSizeIn)) {
                    reader.error("Couldn't parse SpriteGroup. Expected: ':SpriteGroup identifier filename.png SWxSH'"_s);
                    return;
                }

                textureAsset = asset_manager().add_asset(AssetType::Texture, filename.release_value());
                spriteSize = spriteSizeIn.release_value();

                s32 spriteCount = reader.count_occurrences_of_property_in_current_command("sprite"_s);
                if (spriteCount < 1) {
                    reader.error("SpriteGroup must contain at least 1 sprite!"_s);
                    return;
                }
                spriteGroup = add_sprite_group(name.release_value(), spriteCount);
                spriteIndex = 0;

                addChildAsset(&metadata, spriteGroup);
            } else {
                reader.error("Unrecognised command: '{0}'"_s, { command });
                return;
            }
        } else // Properties!
        {
            if (spriteGroup == nullptr) {
                reader.error("Found a property outside of a :SpriteGroup!"_s);
                return;
            } else if (command == "border"_s) {
                auto borderW = reader.read_int<s32>();
                auto borderH = reader.read_int<s32>();
                if (borderW.has_value() && borderH.has_value()) {
                    spriteBorder = v2i(borderW.release_value(), borderH.release_value());
                } else {
                    reader.error("Couldn't parse border. Expected 'border width height'."_s);
                    return;
                }
            } else if (command == "sprite"_s) {
                auto mx = reader.read_int<s32>();
                auto my = reader.read_int<s32>();

                if (mx.has_value() && my.has_value()) {
                    s32 x = mx.release_value();
                    s32 y = my.release_value();

                    auto& group_asset = dynamic_cast<DeprecatedAsset&>(*spriteGroup->loaded_asset);
                    Sprite* sprite = group_asset.spriteGroup.sprites + spriteIndex;
                    sprite->texture = textureAsset;
                    sprite->uv = { spriteBorder.x + x * (spriteSize.x + spriteBorder.x + spriteBorder.x),
                        spriteBorder.y + y * (spriteSize.y + spriteBorder.y + spriteBorder.y),
                        spriteSize.x, spriteSize.y };
                    sprite->pixelWidth = spriteSize.x;
                    sprite->pixelHeight = spriteSize.y;

                    spriteIndex++;
                } else {
                    reader.error("Couldn't parse {0}. Expected '{0} x y'."_s, { command });
                    return;
                }
            } else {
                reader.error("Unrecognised command '{0}'"_s, { command });
                return;
            }
        }
    }

    // Load all the sprites, now that we know their properties are all set.
    for (auto& child : metadata.children) {
        auto& sprite_group_metadata = child.get();
        if (sprite_group_metadata.type != AssetType::Sprite)
            continue;

        auto& sprite_group_asset = dynamic_cast<DeprecatedAsset&>(*sprite_group_metadata.loaded_asset);
        auto& sprite_group = sprite_group_asset.spriteGroup;

        // Convert UVs from pixel space to 0-1 space
        for (s32 i = 0; i < sprite_group.count; i++) {
            Sprite* sprite = sprite_group.sprites + i;
            AssetMetadata* texture_metadata = sprite->texture;
            texture_metadata->ensure_is_loaded();
            auto& texture = dynamic_cast<DeprecatedAsset&>(*texture_metadata->loaded_asset).texture;
            float textureWidth = texture.surface->w;
            float textureHeight = texture.surface->h;

            sprite->uv = {
                sprite->uv.x() / textureWidth,
                sprite->uv.y() / textureHeight,
                sprite->uv.width() / textureWidth,
                sprite->uv.height() / textureHeight
            };
        }
    }
}

ErrorOr<NonnullOwnPtr<Asset>> DeprecatedAssetLoader::load_asset(AssetMetadata& metadata, Blob file_data)
{
    // Type-specific loading
    auto asset = adopt_own(*new DeprecatedAsset);

    switch (metadata.type) {
    case AssetType::BitmapFont: {
        if (BitmapFont::load_from_bmf_data(file_data, metadata, *asset))
            return { move(asset) };

        return Error { "Failed to load bitmap font"_s };
    }

    case AssetType::BuildingDefs: {
        loadBuildingDefs(file_data, metadata, *asset);
        return { move(asset) };
    }

    case AssetType::CursorDefs: {
        load_cursor_defs(file_data, metadata, *asset);
        return { move(asset) };
    }

    case AssetType::DevKeymap: {
        if (globalConsole != nullptr) {
            // NB: We keep the keymap file in the asset memory, so that the CommandShortcut.command can
            // directly refer to the string data from the file, instead of having to assetsAllocate a copy
            // and not be able to free it ever. This is more memory efficient.
            copyFileIntoAsset(&file_data, *asset);
            loadConsoleKeyboardShortcuts(globalConsole, file_data, metadata.shortName);
        }
        return { move(asset) };
    }

    case AssetType::PaletteDefs: {
        load_palette_defs(file_data, metadata, *asset);
        return { move(asset) };
    }

    case AssetType::Shader: {
        copyFileIntoAsset(&file_data, *asset);
        String::from_blob(file_data).value().split_in_two('$', &asset->shader.vertexShader, &asset->shader.fragmentShader);
        return { move(asset) };
    }

    case AssetType::SpriteDefs: {
        load_sprite_defs(file_data, metadata, *asset);
        return { move(asset) };
    }

    case AssetType::TerrainDefs: {
        loadTerrainDefs(file_data, metadata, *asset);
        return { move(asset) };
    }

    case AssetType::TextDocument: {
        // FIXME: Combine things into one allocation?
        copyFileIntoAsset(&file_data, *asset);
        LineReader reader { metadata.shortName, asset->data, {} };
        auto line_count = reader.line_count();
        auto lines_data = assetsAllocate(&asset_manager(), line_count * sizeof(TextDocument::Line));
        auto lines_array = makeArray(line_count, reinterpret_cast<TextDocument::Line*>(lines_data.writable_data()));
        while (reader.load_next_line()) {
            auto line = reader.current_line();
            lines_array.append({ .text = line });
        }
        asset->text_document = TextDocument { lines_data, move(lines_array) };
        return { move(asset) };
    }

    case AssetType::Texts: {
        // Only load assets that match our locale
        if (metadata.locale == get_locale()) {
            asset->texts.isFallbackLocale = false;
        } else if (metadata.locale == Locale::En) {
            logInfo("Loading asset {0} as a default-locale fallback. (Locale {1}, current is {2})"_s, { metadata.fullName, to_string(metadata.locale.value()), to_string(get_locale()) });
            asset->texts.isFallbackLocale = true;
        }

        HashTable<String>* textsTable = (asset->texts.isFallbackLocale ? &asset_manager().defaultTexts : &asset_manager().texts);
        loadTexts(textsTable, &metadata, file_data, *asset);
        return { move(asset) };
    }

    case AssetType::Texture: {
        // TODO: Emergency debug texture that's used if loading a file fails.
        // Right now, we just crash! (Not shippable)
        SDL_Surface* surface = createSurfaceFromFileData(file_data, metadata.fullName);
        if (surface->format->BytesPerPixel != 4) {
            return Error { myprintf("Texture asset '{0}' is not 32bit, which is all we support right now. (BytesPerPixel = {1})"_s, { metadata.shortName, formatInt(surface->format->BytesPerPixel) }) };
        }

        // Premultiply alpha
        // NOTE: We always assume the data isn't premultiplied.
        u32 Rmask = surface->format->Rmask,
            Gmask = surface->format->Gmask,
            Bmask = surface->format->Bmask,
            Amask = surface->format->Amask;
        float rRmask = (float)Rmask,
              rGmask = (float)Gmask,
              rBmask = (float)Bmask,
              rAmask = (float)Amask;

        u32 pixelCount = surface->w * surface->h;
        for (u32 pixelIndex = 0;
            pixelIndex < pixelCount;
            pixelIndex++) {
            u32 pixel = ((u32*)surface->pixels)[pixelIndex];
            float rr = (float)(pixel & Rmask) / rRmask;
            float rg = (float)(pixel & Gmask) / rGmask;
            float rb = (float)(pixel & Bmask) / rBmask;
            float ra = (float)(pixel & Amask) / rAmask;

            u32 r = (u32)(rr * ra * rRmask) & Rmask;
            u32 g = (u32)(rg * ra * rGmask) & Gmask;
            u32 b = (u32)(rb * ra * rBmask) & Bmask;
            u32 a = (u32)(ra * rAmask) & Amask;

            ((u32*)surface->pixels)[pixelIndex] = r | g | b | a;
        }

        asset->texture.surface = surface;
        return { move(asset) };
    }

    case AssetType::UITheme: {
        loadUITheme(file_data, metadata, *asset);
        return { move(asset) };
    }

    default: {
        if (metadata.flags.has(AssetFlags::IsAFile))
            copyFileIntoAsset(&file_data, *asset);

        return { move(asset) };
    }
    }
}
