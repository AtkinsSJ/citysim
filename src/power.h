/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "building.h"
#include "dirty.h"
#include "sector.h"

struct PowerGroup {
    s32 production;
    s32 consumption;

    // TODO: @Size These are always either 1-wide or 1-tall, and up to sectorSize in the other direction, so we could use a much smaller struct than Rect2I!
    ChunkedArray<Rect2I> sectorBoundaries; // Places in nighbouring sectors that are adjacent to this PowerGroup
    s32 networkID;

    ChunkedArray<BuildingRef> buildings;
};

struct PowerSector {
    Rect2I bounds;

    // 0 = none, >0 = any tile with the same value is connected
    // POWER_GROUP_UNKNOWN is used as a temporary value while recalculating
    Array2<u8> tilePowerGroup;

    // NB: Power groups start at 1, (0 means "none") so subtract 1 from the value in tilePowerGroup to get the index!
    ChunkedArray<PowerGroup> powerGroups;
};

struct PowerNetwork {
    s32 id;

    ChunkedArray<PowerGroup*> groups;

    s32 cachedProduction;
    s32 cachedConsumption;
};

struct PowerLayer {
    DirtyRects dirtyRects;

    SectorGrid<PowerSector> sectors;

    u8 powerMaxDistance;
    Array2<u8> tilePowerDistance;

    ChunkedArray<PowerNetwork> networks;

    ArrayChunkPool<PowerGroup> powerGroupsChunkPool;
    ArrayChunkPool<PowerGroup*> powerGroupPointersChunkPool;

    ChunkedArray<BuildingRef> powerBuildings;

    s32 cachedCombinedProduction;
    s32 cachedCombinedConsumption;
};

u8 const POWER_GROUP_UNKNOWN = 255;
struct City;

// Public API
void initPowerLayer(PowerLayer* layer, City* city, MemoryArena* gameArena);
void updatePowerLayer(City* city, PowerLayer* layer);
void markPowerLayerDirty(PowerLayer* layer, Rect2I area);
bool doesTileHavePowerNetwork(City* city, s32 x, s32 y);
PowerNetwork* getPowerNetworkAt(City* city, s32 x, s32 y);
u8 getDistanceToPower(City* city, s32 x, s32 y);

u8 calculatePowerOverlayForTile(City* city, s32 x, s32 y);

void notifyNewBuilding(PowerLayer* layer, BuildingDef* def, Building* building);
void notifyBuildingDemolished(PowerLayer* layer, BuildingDef* def, Building* building);

void debugInspectPower(UI::Panel* panel, City* city, s32 x, s32 y);

// Private-but-actually-still-accessible API
PowerNetwork* newPowerNetwork(PowerLayer* layer);
void freePowerNetwork(PowerNetwork* network);

u8 getPowerGroupID(PowerSector* sector, s32 relX, s32 relY);
void setPowerGroupID(PowerSector* sector, s32 relX, s32 relY, u8 value);
PowerGroup* getPowerGroupAt(PowerSector* sector, s32 relX, s32 relY);

void updateSectorPowerValues(City* city, PowerSector* sector);

void floodFillSectorPowerGroup(PowerSector* sector, s32 x, s32 y, u8 fillValue);
void setRectPowerGroupUnknown(PowerSector* sector, Rect2I area);
void recalculateSectorPowerGroups(City* city, PowerSector* sector);
void floodFillCityPowerNetwork(PowerLayer* powerLayer, PowerGroup* powerGroup, PowerNetwork* network);
void recalculatePowerConnectivity(PowerLayer* powerLayer);
