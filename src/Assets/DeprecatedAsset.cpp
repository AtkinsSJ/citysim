/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DeprecatedAsset.h"
#include <Assets/AssetManager.h>
#include <Assets/AssetMetadata.h>
#include <Debug/Console.h>
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
    assets.fileExtensionToType.put(assets.assetStrings.intern("terrain"_s), AssetType::TerrainDefs);

    assets.directoryNameToType.put(assets.assetStrings.intern("locale"_s), AssetType::Texts);

    assets.asset_loaders_by_type[AssetType::BuildingDefs] = this;
    assets.asset_loaders_by_type[AssetType::TerrainDefs] = this;
    assets.asset_loaders_by_type[AssetType::Texts] = this;
}

void DeprecatedAssetLoader::create_placeholder_assets(AssetManager& assets)
{
    // BuildingDefs
    make_placeholder_asset(assets, AssetType::BuildingDefs);

    // TerrainDefs
    make_placeholder_asset(assets, AssetType::TerrainDefs);

    // Texts
    make_placeholder_asset(assets, AssetType::Texts);
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

    default: {
        VERIFY_NOT_REACHED();
    }
    }
}

}
