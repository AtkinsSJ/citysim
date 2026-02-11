/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DeprecatedAsset.h"
#include <Assets/AssetManager.h>
#include <Assets/AssetMetadata.h>
#include <Debug/Console.h>
#include <IO/File.h>
#include <SDL_image.h>
#include <Settings/Settings.h>
#include <Sim/BuildingCatalogue.h>
#include <Sim/TerrainCatalogue.h>
#include <Util/StringBuilder.h>

namespace Assets {

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
    asset.data = assets_allocate(fileData->size());
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
    asset.data = assets_allocate(file_data.size() + key_array_size);
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

    case AssetType::TerrainDefs: {
        // Remove all of our terrain defs
        removeTerrainDefs(terrainDefs.terrainIDs);
        terrainDefs.terrainIDs = makeEmptyArray<String>();
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

    assets_deallocate(data);
}

static DeprecatedAsset& make_placeholder_asset(AssetManager& assets, AssetType type)
{
    auto* asset = new DeprecatedAsset;
    assets.set_placeholder_asset(type, adopt_own(*asset));
    return *asset;
}

void DeprecatedAssetLoader::register_types(AssetManager& assets)
{
    assets.fileExtensionToType.put(assets.assetStrings.intern("buildings"_s), AssetType::BuildingDefs);
    assets.fileExtensionToType.put(assets.assetStrings.intern("keymap"_s), AssetType::DevKeymap);
    assets.fileExtensionToType.put(assets.assetStrings.intern("terrain"_s), AssetType::TerrainDefs);
    assets.fileExtensionToType.put(assets.assetStrings.intern("theme"_s), AssetType::UITheme);

    assets.directoryNameToType.put(assets.assetStrings.intern("shaders"_s), AssetType::Shader);
    assets.directoryNameToType.put(assets.assetStrings.intern("textures"_s), AssetType::Texture);
    assets.directoryNameToType.put(assets.assetStrings.intern("locale"_s), AssetType::Texts);

    assets.asset_loaders_by_type[AssetType::BuildingDefs] = this;
    assets.asset_loaders_by_type[AssetType::DevKeymap] = this;
    assets.asset_loaders_by_type[AssetType::Shader] = this;
    assets.asset_loaders_by_type[AssetType::TerrainDefs] = this;
    assets.asset_loaders_by_type[AssetType::Texts] = this;
    assets.asset_loaders_by_type[AssetType::Texture] = this;
    assets.asset_loaders_by_type[AssetType::UITheme] = this;
}

void DeprecatedAssetLoader::create_placeholder_assets(AssetManager& assets)
{
    // BuildingDefs
    make_placeholder_asset(assets, AssetType::BuildingDefs);

    // DevKeymap
    make_placeholder_asset(assets, AssetType::DevKeymap);

    // Shader
    make_placeholder_asset(assets, AssetType::Shader);

    // TerrainDefs
    make_placeholder_asset(assets, AssetType::TerrainDefs);

    // Texts
    make_placeholder_asset(assets, AssetType::Texts);

    // Texture
    auto& placeholderTexture = make_placeholder_asset(assets, AssetType::Texture);
    placeholderTexture.data = assets_allocate(2 * 2 * sizeof(u32));
    u32* pixels = (u32*)placeholderTexture.data.writable_data();
    pixels[0] = pixels[3] = 0xffff00ff;
    pixels[1] = pixels[2] = 0xff000000;
    placeholderTexture.texture.surface = SDL_CreateRGBSurfaceFrom(pixels, 2, 2, 32, 2 * sizeof(u32),
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

    // UITheme
    make_placeholder_asset(assets, AssetType::UITheme);
}

ErrorOr<NonnullOwnPtr<Asset>> DeprecatedAssetLoader::load_asset(AssetMetadata& metadata, Blob file_data)
{
    // Type-specific loading
    auto asset = adopt_own(*new DeprecatedAsset);

    switch (metadata.type) {
    case AssetType::BuildingDefs: {
        loadBuildingDefs(file_data, metadata, *asset);
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

    case AssetType::Shader: {
        copyFileIntoAsset(&file_data, *asset);
        String::from_blob(file_data).value().split_in_two('$', &asset->shader.vertexShader, &asset->shader.fragmentShader);
        return { move(asset) };
    }

    case AssetType::TerrainDefs: {
        loadTerrainDefs(file_data, metadata, *asset);
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
        VERIFY_NOT_REACHED();
    }
    }
}

}
