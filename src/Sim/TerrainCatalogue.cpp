/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "TerrainCatalogue.h"
#include <App/App.h>
#include <Debug/Debug.h>
#include <Sim/City.h>
#include <Sim/Game.h>

static TerrainCatalogue s_terrain_catalogue = {};

TerrainCatalogue& TerrainCatalogue::the()
{
    return s_terrain_catalogue;
}

void initTerrainCatalogue(MemoryArena& arena)
{
    initOccupancyArray(&s_terrain_catalogue.terrainDefs, &arena, 128);
    (void)s_terrain_catalogue.terrainDefs.append(); // Null terrain def

    s_terrain_catalogue.terrainNameToType.put({}, 0);

    asset_manager().register_listener(&s_terrain_catalogue);
}

TerrainDef const& TerrainCatalogue::get_def(TerrainType terrain_type) const
{
    if (terrain_type > 0 && terrain_type < s_terrain_catalogue.terrainDefs.count) {
        TerrainDef* found = s_terrain_catalogue.terrainDefs.get(terrain_type);
        if (found != nullptr)
            return *found;
    }

    return *s_terrain_catalogue.terrainDefs.get(0);
}

TerrainType findTerrainTypeByName(String name)
{
    DEBUG_FUNCTION();

    TerrainType result = 0;

    auto def = s_terrain_catalogue.terrainDefsByName.find_value(name);
    if (def.has_value() && def != nullptr) {
        result = def.value()->typeID;
    }

    return result;
}

void saveTerrainTypes()
{
    s_terrain_catalogue.terrainNameToOldType.put_all(s_terrain_catalogue.terrainNameToType);
}

void TerrainCatalogue::remap_terrain_types(City& city)
{
    // FIXME: This doesn't seem to work any more. Terrain doesn't update. Investigate!

    // First, remap any Names that are not present in the current data, so they won't get
    // merged accidentally.
    for (auto it = terrainNameToOldType.iterate(); it.hasNext(); it.next()) {
        auto entry = it.getEntry();
        terrainNameToType.ensure(entry->key, (TerrainType)terrainNameToType.count());
    }

    if (terrainNameToOldType.count() > 0) {
        Array<TerrainType> oldTypeToNewType = temp_arena().allocate_array<TerrainType>(terrainNameToOldType.count(), true);
        for (auto it = terrainNameToOldType.iterate(); it.hasNext(); it.next()) {
            auto entry = it.getEntry();
            String terrainName = entry->key;
            TerrainType oldType = entry->value;

            oldTypeToNewType[oldType] = terrainNameToType.find_value(terrainName).value_or(0);
        }

        TerrainLayer& layer = city.terrainLayer;

        for (s32 y = 0; y < layer.m_tile_terrain_type.height(); y++) {
            for (s32 x = 0; x < layer.m_tile_terrain_type.width(); x++) {
                TerrainType oldType = layer.m_tile_terrain_type.get(x, y);

                if (oldType < oldTypeToNewType.count && (oldTypeToNewType[oldType] != 0)) {
                    layer.m_tile_terrain_type.set(x, y, oldTypeToNewType[oldType]);
                }
            }
        }
    }

    saveTerrainTypes();
}

void TerrainCatalogue::after_assets_loaded()
{
    if (auto* game_scene = dynamic_cast<GameScene*>(&App::the().scene()))
        remap_terrain_types(game_scene->state().city);
}
