/*
 * Copyright (c) 2018-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "dirty.h"
#include "sector.h"
#include <Util/BitArray.h>
#include <Util/Flags.h>

struct City;

enum class ZoneType : u8 {
    None,

    Residential,
    Commercial,
    Industrial,

    COUNT,
};
constexpr ZoneType FirstZoneType = ZoneType::Residential;

struct ZoneDef {
    ZoneType typeID;
    String textAssetName;
    Colour color;
    s32 costPerTile;
    bool carriesPower;
    s32 maximumDistanceToRoad;
};

EnumMap<ZoneType, ZoneDef> const ZONE_DEFS {
    { ZoneType::None, "zone_none"_s, Colour::from_rgb_255(255, 255, 255, 128), 10, false, 0 },
    { ZoneType::Residential, "zone_residential"_s, Colour::from_rgb_255(0, 255, 0, 128), 10, true, 3 },
    { ZoneType::Commercial, "zone_commercial"_s, Colour::from_rgb_255(0, 0, 255, 128), 10, true, 2 },
    { ZoneType::Industrial, "zone_industrial"_s, Colour::from_rgb_255(255, 255, 0, 128), 20, true, 4 },
};

enum class ZoneSectorFlags : u8 {
    HasResZones,
    HasComZones,
    HasIndZones,
    HasEmptyResZones,
    HasEmptyComZones,
    HasEmptyIndZones,
    COUNT,
};

struct ZoneSector {
    Rect2I bounds;
    Flags<ZoneSectorFlags> zoneSectorFlags;

    EnumMap<ZoneType, f32> averageDesirability;
};

struct ZoneLayer {
    Array2<ZoneType> tileZone;
    EnumMap<ZoneType, Array2<u8>> tileDesirability;

    SectorGrid<ZoneSector> sectors;

    EnumMap<ZoneType, BitArray> sectorsWithZones;
    EnumMap<ZoneType, BitArray> sectorsWithEmptyZones;

    EnumMap<ZoneType, Array<s32>> mostDesirableSectors;

    EnumMap<ZoneType, s32> population; // NB: ZoneType::None is used for jobs provided by non-zone, city buildings

    // Calculated every so often
    EnumMap<ZoneType, s32> demand;
};

struct CanZoneQuery {
    Rect2I bounds;
    ZoneDef const* zoneDef;

    s32 zoneableTilesCount;
    u8* tileCanBeZoned; // NB: is either 0 or 1
};

void initZoneLayer(ZoneLayer* zoneLayer, City* city, MemoryArena* gameArena);
void updateZoneLayer(City* city, ZoneLayer* layer);
void calculateDemand(City* city, ZoneLayer* layer);

CanZoneQuery* queryCanZoneTiles(City* city, ZoneType zoneType, Rect2I bounds);
bool canZoneTile(CanZoneQuery* query, s32 x, s32 y);
s32 calculateZoneCost(CanZoneQuery* query);

void placeZone(City* city, ZoneType zoneType, Rect2I area);
void markZonesAsEmpty(City* city, Rect2I footprint);
ZoneType getZoneAt(City* city, s32 x, s32 y);

void drawZones(City* city, Rect2I visibleArea, s8 shaderID);

void growSomeZoneBuildings(City* city);
bool isZoneAcceptable(City* city, ZoneType zoneType, s32 x, s32 y);

s32 getTotalResidents(City* city);
s32 getTotalJobs(City* city);

void saveZoneLayer(ZoneLayer* layer, struct BinaryFileWriter* writer);
bool loadZoneLayer(ZoneLayer* layer, City* city, struct BinaryFileReader* reader);
