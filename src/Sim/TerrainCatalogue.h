/*
 * Copyright (c) 2025-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetManagerListener.h>
#include <Assets/Forward.h>
#include <Sim/Forward.h>
#include <Util/HashMap.h>
#include <Util/OccupancyArray.h>
#include <Util/StringTable.h>

using TerrainType = u8;
struct TerrainDef {
    String name;
    TerrainType typeID;

    String textAssetName;

    String spriteName;

    bool canBuildOn;

    Array<String> borderSpriteNames;

    bool drawBordersOver;
};

struct TerrainCatalogue final : public AssetManagerListener {
    static TerrainCatalogue& the();

    TerrainDef const& get_def(TerrainType terrain_type) const;

    OccupancyArray<TerrainDef> terrainDefs;

    HashMap<String, TerrainDef*> terrainDefsByName { 128 };
    StringTable terrainNames;

    HashMap<String, TerrainType> terrainNameToOldType { 128 };
    HashMap<String, TerrainType> terrainNameToType { 128 };

    // ^AssetManagerListener
    virtual void after_assets_loaded() override;

    void remap_terrain_types(City&);
};

void initTerrainCatalogue(MemoryArena&);
void loadTerrainDefs(Blob data, AssetMetadata& metadata, DeprecatedAsset& asset);
void removeTerrainDefs(Array<String> namesToRemove);

// Returns 0 if not found
TerrainType findTerrainTypeByName(String name);

void saveTerrainTypes();
