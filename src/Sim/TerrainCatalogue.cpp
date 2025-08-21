/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "TerrainCatalogue.h"
#include "AppState.h"
#include <Assets/Asset.h>
#include <Debug/Debug.h>
#include <IO/LineReader.h>
#include <Sim/City.h>

static TerrainCatalogue s_terrain_catalogue = {};

TerrainCatalogue& TerrainCatalogue::the()
{
    return s_terrain_catalogue;
}

void initTerrainCatalogue()
{
    initOccupancyArray(&s_terrain_catalogue.terrainDefs, &AppState::the().systemArena, 128);
    s_terrain_catalogue.terrainDefs.append(); // Null terrain def

    s_terrain_catalogue.terrainNameToType.put(nullString, 0);

    asset_manager().register_listener(&s_terrain_catalogue);
}

void loadTerrainDefs(Blob data, Asset* asset)
{
    DEBUG_FUNCTION();

    LineReader reader { asset->shortName, data };

    // Pre scan for the number of Terrains, so we can allocate enough space in the asset.
    s32 terrainCount = 0;
    while (reader.load_next_line()) {
        String command = reader.next_token();
        if (command == ":Terrain"_s) {
            terrainCount++;
        }
    }

    asset->data = assetsAllocate(&asset_manager(), sizeof(String) * terrainCount);
    asset->terrainDefs.terrainIDs = makeArray(terrainCount, (String*)asset->data.writable_data());

    reader.restart();

    TerrainDef* def = nullptr;

    while (reader.load_next_line()) {
        String firstWord = reader.next_token();

        if (firstWord.chars[0] == ':') // Definitions
        {
            // Define something
            firstWord.chars++;
            firstWord.length--;

            if (firstWord == "Terrain"_s) {
                String name = reader.next_token();
                if (name.is_empty()) {
                    reader.error("Couldn't parse Terrain. Expected: ':Terrain identifier'"_s);
                    return;
                }

                Indexed<TerrainDef> slot = s_terrain_catalogue.terrainDefs.append();
                def = &slot.value();

                if (slot.index() > u8Max) {
                    reader.error("Too many Terrain definitions! The most we support is {0}."_s, { formatInt(u8Max) });
                    return;
                }
                def->typeID = (u8)slot.index();

                def->name = intern(&s_terrain_catalogue.terrainNames, name);
                asset->terrainDefs.terrainIDs.append(def->name);
                s_terrain_catalogue.terrainDefsByName.put(def->name, def);
                s_terrain_catalogue.terrainNameToType.put(def->name, def->typeID);
            } else {
                reader.error("Unrecognised command: '{0}'"_s, { firstWord });
            }
        } else // Properties!
        {
            if (def == nullptr) {
                reader.error("Found a property before starting a :Terrain!"_s);
                return;
            } else if (firstWord == "borders"_s) {
                def->borderSpriteNames = asset_manager().arena.allocate_array<String>(80);
            } else if (firstWord == "border"_s) {
                def->borderSpriteNames.append(intern(&asset_manager().assetStrings, reader.next_token()));
            } else if (firstWord == "can_build_on"_s) {
                if (auto maybe_bool = reader.read_bool(); maybe_bool.has_value())
                    def->canBuildOn = maybe_bool.release_value();
            } else if (firstWord == "draw_borders_over"_s) {
                if (auto maybe_bool = reader.read_bool(); maybe_bool.has_value())
                    def->drawBordersOver = maybe_bool.release_value();
            } else if (firstWord == "name"_s) {
                def->textAssetName = intern(&asset_manager().assetStrings, reader.next_token());
            } else if (firstWord == "sprite"_s) {
                def->spriteName = intern(&asset_manager().assetStrings, reader.next_token());
            } else {
                reader.warn("Unrecognised property '{0}' inside command ':Terrain'"_s, { firstWord });
            }
        }
    }
}

void removeTerrainDefs(Array<String> namesToRemove)
{
    for (s32 nameIndex = 0; nameIndex < namesToRemove.count; nameIndex++) {
        String terrainName = namesToRemove[nameIndex];
        s32 terrainIndex = findTerrainTypeByName(terrainName);
        if (terrainIndex > 0) {
            s_terrain_catalogue.terrainDefs.removeIndex(terrainIndex);

            s_terrain_catalogue.terrainNameToType.removeKey(terrainName);
        }
    }
}

TerrainDef* getTerrainDef(u8 terrainType)
{
    TerrainDef* result = s_terrain_catalogue.terrainDefs.get(0);

    if (terrainType > 0 && terrainType < s_terrain_catalogue.terrainDefs.count) {
        TerrainDef* found = s_terrain_catalogue.terrainDefs.get(terrainType);
        if (found != nullptr)
            result = found;
    }

    return result;
}

u8 findTerrainTypeByName(String name)
{
    DEBUG_FUNCTION();

    u8 result = 0;

    Maybe<TerrainDef*> def = s_terrain_catalogue.terrainDefsByName.findValue(name);
    if (def.isValid && def.value != nullptr) {
        result = def.value->typeID;
    }

    return result;
}

void saveTerrainTypes()
{
    s_terrain_catalogue.terrainNameToOldType.putAll(&s_terrain_catalogue.terrainNameToType);
}

void remapTerrainTypes()
{
    // First, remap any Names that are not present in the current data, so they won't get
    // merged accidentally.
    for (auto it = s_terrain_catalogue.terrainNameToOldType.iterate(); it.hasNext(); it.next()) {
        auto entry = it.getEntry();
        if (!s_terrain_catalogue.terrainNameToType.contains(entry->key)) {
            s_terrain_catalogue.terrainNameToType.put(entry->key, (u8)s_terrain_catalogue.terrainNameToType.count);
        }
    }

    if (s_terrain_catalogue.terrainNameToOldType.count > 0) {
        Array<u8> oldTypeToNewType = temp_arena().allocate_array<u8>(s_terrain_catalogue.terrainNameToOldType.count, true);
        for (auto it = s_terrain_catalogue.terrainNameToOldType.iterate(); it.hasNext(); it.next()) {
            auto entry = it.getEntry();
            String terrainName = entry->key;
            u8 oldType = entry->value;

            oldTypeToNewType[oldType] = s_terrain_catalogue.terrainNameToType.findValue(terrainName).orDefault(0);
        }

        TerrainLayer& layer = AppState::the().gameState->city.terrainLayer;

        for (s32 y = 0; y < layer.tileTerrainType.h; y++) {
            for (s32 x = 0; x < layer.tileTerrainType.w; x++) {
                u8 oldType = layer.tileTerrainType.get(x, y);

                if (oldType < oldTypeToNewType.count && (oldTypeToNewType[oldType] != 0)) {
                    layer.tileTerrainType.set(x, y, oldTypeToNewType[oldType]);
                }
            }
        }
    }

    saveTerrainTypes();
}

void TerrainCatalogue::after_assets_loaded()
{
    remapTerrainTypes();
}
