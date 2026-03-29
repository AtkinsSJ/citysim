/*
 * Copyright (c) 2018-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Colour.h>
#include <IO/Forward.h>
#include <Sim/DirtyRects.h>
#include <Sim/Forward.h>
#include <Sim/Sector.h>
#include <Util/BitArray.h>
#include <Util/EnumMap.h>
#include <Util/Flags.h>

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

struct ZoneSector : public BasicSector {
    Flags<ZoneSectorFlags> zoneSectorFlags;

    EnumMap<ZoneType, float> averageDesirability;
};

class ZoneLayer {
public:
    ZoneLayer() = default; // FIXME: Temporary, required by initCity()
    ZoneLayer(City&, MemoryArena&);

    ZoneType get_zone_at(s32 x, s32 y) const;

    void update(City&);
    void draw_zones(Rect2I visible_area, s8 shader_id) const;

    u32 total_residents() const;
    u32 total_jobs() const;

    void save(BinaryFileWriter&) const;
    bool load(BinaryFileReader&);

    Array2<ZoneType> tileZone;
    EnumMap<ZoneType, Array2<u8>> tileDesirability;

    SectorGrid<ZoneSector> sectors;

    EnumMap<ZoneType, BitArray> sectorsWithZones;
    EnumMap<ZoneType, BitArray> sectorsWithEmptyZones;

    EnumMap<ZoneType, Array<s32>> mostDesirableSectors;

    EnumMap<ZoneType, s32> population; // NB: ZoneType::None is used for jobs provided by non-zone, city buildings

    // Calculated every so often
    EnumMap<ZoneType, s32> demand;

private:
    void calculate_demand();
};

struct CanZoneQuery {
    Rect2I bounds;
    ZoneDef const* zoneDef;

    s32 zoneableTilesCount;
    Array2<u8> tileCanBeZoned; // NB: is either 0 or 1

    bool can_zone_tile(s32 x, s32 y) const;
    s32 calculate_zone_cost() const;
};
CanZoneQuery queryCanZoneTiles(City* city, ZoneType zoneType, Rect2I bounds);

void placeZone(City* city, ZoneType zoneType, Rect2I area);

void growSomeZoneBuildings(City* city);
bool isZoneAcceptable(City* city, ZoneType zoneType, s32 x, s32 y);
