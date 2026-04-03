/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "TerrainDefs.h"
#include <Assets/AssetManager.h>
#include <Sim/TerrainCatalogue.h>

ErrorOr<NonnullOwnPtr<TerrainDefs>> TerrainDefs::load(AssetMetadata& metadata, Blob file_data)
{
    DEBUG_FUNCTION();

    LineReader reader { metadata.shortName, file_data };

    // Pre scan for the number of Terrains, so we can allocate enough space in the asset.
    size_t terrainCount = 0;
    while (reader.load_next_line()) {
        if (auto command = reader.next_token(); command == ":Terrain"_s)
            terrainCount++;
    }

    auto terrain_ids = asset_manager().allocate_array<String>(terrainCount);

    reader.restart();

    auto& catalogue = TerrainCatalogue::the();

    TerrainDef* def = nullptr;

    while (reader.load_next_line()) {
        auto maybe_first_word = reader.next_token();
        if (!maybe_first_word.has_value())
            continue;
        auto firstWord = maybe_first_word.release_value();

        if (firstWord.starts_with(':')) // Definitions
        {
            // Define something
            firstWord = firstWord.substring(1).deprecated_to_string();

            if (firstWord == "Terrain"_s) {
                auto name = reader.next_token();
                if (!name.has_value())
                    return reader.make_error_message("Couldn't parse Terrain. Expected: ':Terrain identifier'"_s);

                Indexed<TerrainDef> slot = catalogue.terrainDefs.append();
                def = &slot.value();

                if (slot.index() > u8Max)
                    return reader.make_error_message("Too many Terrain definitions! The most we support is {0}."_s, { formatInt(u8Max) });

                def->typeID = (u8)slot.index();

                def->name = catalogue.terrainNames.intern(name.value());
                terrain_ids.append(def->name);
                catalogue.terrainDefsByName.put(def->name, def);
                catalogue.terrainNameToType.put(def->name, def->typeID);
            } else {
                reader.error("Unrecognised command: '{0}'"_s, { firstWord });
            }
        } else // Properties!
        {
            if (def == nullptr)
                return reader.make_error_message("Found a property before starting a :Terrain!"_s);

            if (firstWord == "borders"_s) {
                def->borderSpriteNames = asset_manager().arena.allocate_array<String>(80);
            } else if (firstWord == "border"_s) {
                if (auto token = reader.next_token(); token.has_value()) {
                    def->borderSpriteNames.append(asset_manager().assetStrings.intern(token.release_value()));
                } else {
                    return reader.make_error_message("Missing sprite name for `border`"_s);
                }
            } else if (firstWord == "can_build_on"_s) {
                if (auto maybe_bool = reader.read_bool(); maybe_bool.has_value())
                    def->canBuildOn = maybe_bool.release_value();
            } else if (firstWord == "draw_borders_over"_s) {
                if (auto maybe_bool = reader.read_bool(); maybe_bool.has_value())
                    def->drawBordersOver = maybe_bool.release_value();
            } else if (firstWord == "name"_s) {
                if (auto token = reader.next_token(); token.has_value()) {
                    def->textAssetName = asset_manager().assetStrings.intern(token.release_value());
                } else {
                    return reader.make_error_message("Missing name for `name`"_s);
                }
            } else if (firstWord == "sprite"_s) {
                if (auto token = reader.next_token(); token.has_value()) {
                    def->spriteName = asset_manager().assetStrings.intern(token.release_value());
                } else {
                    return reader.make_error_message("Missing name for `sprite`"_s);
                }
            } else {
                reader.warn("Unrecognised property '{0}' inside command ':Terrain'"_s, { firstWord });
            }
        }
    }

    return adopt_own(*new TerrainDefs(move(terrain_ids)));
}

TerrainDefs::TerrainDefs(Array<String> terrain_ids)
    : m_terrain_ids(move(terrain_ids))
{
}

void TerrainDefs::unload(AssetMetadata& metadata)
{
    auto& terrain_catalogue = TerrainCatalogue::the();

    for (auto const& terrainName : m_terrain_ids) {
        s32 terrainIndex = findTerrainTypeByName(terrainName);
        if (terrainIndex > 0) {
            terrain_catalogue.terrainDefs.removeIndex(terrainIndex);
            terrain_catalogue.terrainNameToType.remove(terrainName);
        }
    }

    asset_manager().deallocate(m_terrain_ids);
}
