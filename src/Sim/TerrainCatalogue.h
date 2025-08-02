/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Assets/AssetManagerListener.h>
#include <Assets/Forward.h>
#include <Sim/Forward.h>
#include <Util/HashTable.h>
#include <Util/OccupancyArray.h>
#include <Util/StringTable.h>

struct TerrainDef {
    String name;
    u8 typeID;

    String textAssetName;

    String spriteName;

    bool canBuildOn;

    Array<String> borderSpriteNames;

    bool drawBordersOver;
};

struct TerrainCatalogue final : public AssetManagerListener {
    static TerrainCatalogue& the();

    OccupancyArray<TerrainDef> terrainDefs;

    HashTable<TerrainDef*> terrainDefsByName;
    StringTable terrainNames;

    HashTable<u8> terrainNameToOldType;
    HashTable<u8> terrainNameToType;

    // ^AssetManagerListener
    virtual void after_assets_loaded() override;
};

void initTerrainCatalogue();
void loadTerrainDefs(Blob data, Asset* asset);
void removeTerrainDefs(Array<String> namesToRemove);
TerrainDef* getTerrainDef(u8 terrainType);

// Returns 0 if not found
u8 findTerrainTypeByName(String name);

void saveTerrainTypes();
void remapTerrainTypes();
