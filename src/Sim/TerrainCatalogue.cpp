/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "TerrainCatalogue.h"
#include "AppState.h"
#include <Debug/Debug.h>
#include <Sim/City.h>

static TerrainCatalogue s_terrain_catalogue = {};

TerrainCatalogue& TerrainCatalogue::the()
{
    return s_terrain_catalogue;
}

void initTerrainCatalogue()
{
    initOccupancyArray(&s_terrain_catalogue.terrainDefs, &AppState::the().systemArena, 128);
    (void)s_terrain_catalogue.terrainDefs.append(); // Null terrain def

    s_terrain_catalogue.terrainNameToType.put({}, 0);

    asset_manager().register_listener(&s_terrain_catalogue);
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

    auto def = s_terrain_catalogue.terrainDefsByName.find_value(name);
    if (def.has_value() && def != nullptr) {
        result = def.value()->typeID;
    }

    return result;
}

void saveTerrainTypes()
{
    s_terrain_catalogue.terrainNameToOldType.putAll(&s_terrain_catalogue.terrainNameToType);
}

void remapTerrainTypes()
{
    // FIXME: This doesn't seem to work any more. Terrain doesn't update. Investigate!

    // First, remap any Names that are not present in the current data, so they won't get
    // merged accidentally.
    for (auto it = s_terrain_catalogue.terrainNameToOldType.iterate(); it.hasNext(); it.next()) {
        auto entry = it.getEntry();
        if (!s_terrain_catalogue.terrainNameToType.contains(entry->key)) {
            s_terrain_catalogue.terrainNameToType.put(entry->key, (u8)s_terrain_catalogue.terrainNameToType.count());
        }
    }

    if (s_terrain_catalogue.terrainNameToOldType.count() > 0) {
        Array<u8> oldTypeToNewType = temp_arena().allocate_array<u8>(s_terrain_catalogue.terrainNameToOldType.count(), true);
        for (auto it = s_terrain_catalogue.terrainNameToOldType.iterate(); it.hasNext(); it.next()) {
            auto entry = it.getEntry();
            String terrainName = entry->key;
            u8 oldType = entry->value;

            oldTypeToNewType[oldType] = s_terrain_catalogue.terrainNameToType.find_value(terrainName).value_or(0);
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
    if (AppState::the().gameState)
        remapTerrainTypes();
}
