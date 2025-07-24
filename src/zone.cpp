/*
 * Copyright (c) 2018-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "zone.h"
#include "AppState.h"
#include "binary_file_reader.h"
#include "binary_file_writer.h"
#include "city.h"
#include "save_file.h"
#include <Gfx/Renderer.h>

void initZoneLayer(ZoneLayer* zoneLayer, City* city, MemoryArena* gameArena)
{
    zoneLayer->tileZone = gameArena->allocate_array_2d<ZoneType>(city->bounds.w, city->bounds.h);

    initSectorGrid(&zoneLayer->sectors, gameArena, city->bounds.w, city->bounds.h, 16, 8);
    s32 sectorCount = getSectorCount(&zoneLayer->sectors);

    // NB: Element 0 is empty because tracking spots with no zone is not useful
    for (auto zone_type : enum_values<ZoneType>()) {
        initBitArray(&zoneLayer->sectorsWithZones[zone_type], gameArena, sectorCount);
        initBitArray(&zoneLayer->sectorsWithEmptyZones[zone_type], gameArena, sectorCount);

        zoneLayer->tileDesirability[zone_type] = gameArena->allocate_array_2d<u8>(city->bounds.w, city->bounds.h);

        zoneLayer->mostDesirableSectors[zone_type] = gameArena->allocate_array<s32>(sectorCount, true);
        for (s32 sectorIndex = 0; sectorIndex < sectorCount; sectorIndex++) {
            // To start with, we just fill the array 0-to-N because we don't know what's the most desirable.
            zoneLayer->mostDesirableSectors[zone_type][sectorIndex] = sectorIndex;
        }
    }

    for (s32 sectorIndex = 0; sectorIndex < sectorCount; sectorIndex++) {
        ZoneSector* sector = &zoneLayer->sectors.sectors[sectorIndex];

        sector->zoneSectorFlags = 0;
    }
}

ZoneType getZoneAt(City* city, s32 x, s32 y)
{
    return (ZoneType)city->zoneLayer.tileZone.getIfExists(x, y, ZoneType::None);
}

s32 calculateZoneCost(CanZoneQuery* query)
{
    return query->zoneableTilesCount * query->zoneDef->costPerTile;
}

CanZoneQuery* queryCanZoneTiles(City* city, ZoneType zoneType, Rect2I bounds)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Highlight);
    // Assumption made: `bounds` is a valid rectangle within the city's bounds

    // Allocate the Query struct
    CanZoneQuery* query = nullptr;
    s32 tileCount = areaOf(bounds);
    smm structSize = sizeof(CanZoneQuery) + (tileCount * sizeof(query->tileCanBeZoned[0]));
    u8* memory = static_cast<u8*>(temp_arena().allocate_deprecated(structSize));
    query = (CanZoneQuery*)memory;
    query->tileCanBeZoned = memory + sizeof(CanZoneQuery);
    query->bounds = bounds;
    query->zoneDef = &ZONE_DEFS[zoneType];

    // Actually do the calculation

    // For now, you can only zone free tiles, where there are no buildings or obstructions.
    // You CAN zone over other zones though. Just, not if something has already grown in that
    // zone tile.
    // At some point we'll probably want to change this, because my least-favourite thing
    // about SC3K is that clearing a built-up zone means demolishing the buildings then quickly
    // switching to the de-zone tool before something else pops up in the free space!
    // - Sam, 13/12/2018

    for (s32 y = bounds.y;
        y < bounds.y + bounds.h;
        y++) {
        for (s32 x = bounds.x;
            x < bounds.x + bounds.w;
            x++) {
            bool canZone = true;

            // Ignore tiles that are already this zone!
            if (getZoneAt(city, x, y) == zoneType) {
                canZone = false;
            }
            // Terrain must be buildable
            // @Speed: URGH this terrain lookup for every tile is nasty!
            else if (!getTerrainAt(city, x, y)->canBuildOn) {
                canZone = false;
            }
            // Tile must be empty
            else if (buildingExistsAt(city, x, y)) {
                canZone = false;
            }

            // Pos relative to our query
            s32 qX = x - bounds.x;
            s32 qY = y - bounds.y;
            query->tileCanBeZoned[(qY * bounds.w) + qX] = canZone ? 1 : 0;
            if (canZone)
                query->zoneableTilesCount++;
        }
    }

    return query;
}

bool canZoneTile(CanZoneQuery* query, s32 x, s32 y)
{
    ASSERT(contains(query->bounds, x, y));
    s32 qX = x - query->bounds.x;
    s32 qY = y - query->bounds.y;

    return query->tileCanBeZoned[(qY * query->bounds.w) + qX] != 0;
}

void drawZones(City* city, Rect2I visibleTileBounds, s8 shaderID)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::GameUpdate);

    auto& renderer = the_renderer();
    Rect2 spriteBounds = rectXYWH(0.0f, 0.0f, 1.0f, 1.0f);
    Optional<ZoneType> current_zone_type;
    V4 zoneColor = {};

    // TODO: @Speed: Use a drawGrid() call for this somehow!

    // TODO: @Speed: areaOf() is a poor heuristic! It's safely >= the actual value, but it would be better to
    // actually see how many there are. Though that'd be a double-iteration, unless we keep a cached count.
    DrawRectsGroup* group = beginRectsGroupUntextured(&renderer.world_buffer(), shaderID, areaOf(visibleTileBounds));

    for (s32 y = visibleTileBounds.y;
        y < visibleTileBounds.y + visibleTileBounds.h;
        y++) {
        spriteBounds.y = (f32)y;
        for (s32 x = visibleTileBounds.x;
            x < visibleTileBounds.x + visibleTileBounds.w;
            x++) {
            ZoneType tile_zone = getZoneAt(city, x, y);
            if (tile_zone != ZoneType::None) {
                if (tile_zone != current_zone_type) {
                    current_zone_type = tile_zone;
                    zoneColor = ZONE_DEFS[current_zone_type.value()].color;
                }

                spriteBounds.x = (f32)x;

                addUntexturedRect(group, spriteBounds, zoneColor);
            }
        }
    }

    endRectsGroup(group);
}

void placeZone(City* city, ZoneType zoneType, Rect2I area)
{
    DEBUG_FUNCTION();

    ZoneLayer* zoneLayer = &city->zoneLayer;

    for (s32 y = area.y;
        y < area.y + area.h;
        y++) {
        for (s32 x = area.x;
            x < area.x + area.w;
            x++) {
            // Ignore tiles that are already this zone!
            if ((zoneLayer->tileZone.get(x, y) != zoneType)
                // Terrain must be buildable
                // @Speed: URGH this terrain lookup for every tile is nasty!
                && (getTerrainAt(city, x, y)->canBuildOn)
                && (!buildingExistsAt(city, x, y))) {
                zoneLayer->tileZone.set(x, y, zoneType);
            }
        }
    }

    // TODO: mark the affected zone sectors as dirty

    // Zones carry power!
    markAreaDirty(city, area);
}

void markZonesAsEmpty(City* /*city*/, Rect2I /*footprint*/)
{
    DEBUG_FUNCTION();

    // This now doesn't do anything but I'm keeping it because we probably want dirty rects/local updates later.
}

void updateZoneLayer(City* city, ZoneLayer* layer)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    for (s32 i = 0; i < layer->sectors.sectorsToUpdatePerTick; i++) {
        Indexed<ZoneSector*> sectorWithIndex = getNextSectorWithIndex(&layer->sectors);
        ZoneSector* sector = sectorWithIndex.value;
        s32 sectorIndex = sectorWithIndex.index;

        // What zones does it contain?
        {
            DEBUG_BLOCK_T("updateZoneLayer: zone contents", DebugCodeDataTag::Simulation);

            for (auto zone_type : enum_values<ZoneType>()) {
                layer->sectorsWithZones[zone_type].unsetBit(sectorIndex);
                layer->sectorsWithEmptyZones[zone_type].unsetBit(sectorIndex);
            }

            sector->zoneSectorFlags = 0;

            for (s32 y = sector->bounds.y; y < sector->bounds.y + sector->bounds.h; y++) {
                for (s32 x = sector->bounds.x; x < sector->bounds.x + sector->bounds.w; x++) {
                    ZoneType zone = getZoneAt(city, x, y);
                    if (zone == ZoneType::None)
                        continue;

                    bool isFilled = buildingExistsAt(city, x, y);
                    switch (zone) {
                    case ZoneType::Residential: {
                        sector->zoneSectorFlags |= ZoneSector_HasResZones;
                        if (!isFilled)
                            sector->zoneSectorFlags |= ZoneSector_HasEmptyResZones;
                    } break;

                    case ZoneType::Commercial: {
                        sector->zoneSectorFlags |= ZoneSector_HasComZones;
                        if (!isFilled)
                            sector->zoneSectorFlags |= ZoneSector_HasEmptyComZones;
                    } break;

                    case ZoneType::Industrial: {
                        sector->zoneSectorFlags |= ZoneSector_HasIndZones;
                        if (!isFilled)
                            sector->zoneSectorFlags |= ZoneSector_HasEmptyIndZones;
                    } break;

                        INVALID_DEFAULT_CASE;
                    }
                }
            }

            if (sector->zoneSectorFlags & ZoneSector_HasResZones) {
                layer->sectorsWithZones[ZoneType::Residential].setBit(sectorIndex);
            }
            if (sector->zoneSectorFlags & ZoneSector_HasEmptyResZones) {
                layer->sectorsWithEmptyZones[ZoneType::Residential].setBit(sectorIndex);
            }
            if (sector->zoneSectorFlags & ZoneSector_HasComZones) {
                layer->sectorsWithZones[ZoneType::Commercial].setBit(sectorIndex);
            }
            if (sector->zoneSectorFlags & ZoneSector_HasEmptyComZones) {
                layer->sectorsWithEmptyZones[ZoneType::Commercial].setBit(sectorIndex);
            }
            if (sector->zoneSectorFlags & ZoneSector_HasIndZones) {
                layer->sectorsWithZones[ZoneType::Industrial].setBit(sectorIndex);
            }
            if (sector->zoneSectorFlags & ZoneSector_HasEmptyIndZones) {
                layer->sectorsWithEmptyZones[ZoneType::Industrial].setBit(sectorIndex);
            }
        }

        // What's the desirability?
        {
            DEBUG_BLOCK_T("updateZoneLayer: desirability", DebugCodeDataTag::Simulation);

            f32 totalResDesirability = 0.0f;
            f32 totalComDesirability = 0.0f;
            f32 totalIndDesirability = 0.0f;

            for (s32 y = sector->bounds.y; y < sector->bounds.y + sector->bounds.h; y++) {
                for (s32 x = sector->bounds.x; x < sector->bounds.x + sector->bounds.w; x++) {
                    // Residential
                    {
                        // Land value = good
                        f32 desirability = getLandValuePercentAt(city, x, y);

                        // Health coverage = good
                        desirability += getHealthCoveragePercentAt(city, x, y) * 0.3f;

                        // Fire protection = good
                        desirability += getFireProtectionPercentAt(city, x, y) * 0.2f;

                        // Police coverage = good
                        desirability += getPoliceCoveragePercentAt(city, x, y) * 0.3f;

                        // pollution = bad
                        desirability -= getPollutionPercentAt(city, x, y) * 0.4f;

                        layer->tileDesirability[ZoneType::Residential].set(x, y, clamp01AndMap_u8(desirability));

                        totalResDesirability += desirability;
                    }

                    // Commercial
                    {
                        // Land value = very good
                        f32 desirability = getLandValuePercentAt(city, x, y) * 2.0f;

                        // Fire protection = good
                        desirability += getFireProtectionPercentAt(city, x, y) * 0.2f;

                        // Police coverage = good
                        desirability += getPoliceCoveragePercentAt(city, x, y) * 0.3f;

                        // pollution = bad
                        desirability -= getPollutionPercentAt(city, x, y) * 0.2f;

                        layer->tileDesirability[ZoneType::Commercial].set(x, y, clamp01AndMap_u8(desirability));

                        totalComDesirability += desirability;
                    }

                    // Industrial
                    {
                        // Lower land value is better
                        f32 desirability = 1.0f - getLandValuePercentAt(city, x, y);

                        // Fire protection = good
                        desirability += getFireProtectionPercentAt(city, x, y) * 0.2f;

                        // Police coverage = good
                        desirability += getPoliceCoveragePercentAt(city, x, y) * 0.2f;

                        // pollution = slightly bad
                        desirability -= getPollutionPercentAt(city, x, y) * 0.15f;

                        layer->tileDesirability[ZoneType::Industrial].set(x, y, clamp01AndMap_u8(desirability));

                        totalIndDesirability += desirability;
                    }
                }
            }

            s32 sectorArea = areaOf(sector->bounds);
            f32 invSectorArea = 1.0f / (f32)sectorArea;
            sector->averageDesirability[ZoneType::Residential] = totalResDesirability * invSectorArea;
            sector->averageDesirability[ZoneType::Commercial] = totalComDesirability * invSectorArea;
            sector->averageDesirability[ZoneType::Industrial] = totalIndDesirability * invSectorArea;
        }
    }

    // Sort the mostDesirableSectors array
    for (auto zone_type : enum_values<ZoneType>()) {
        layer->mostDesirableSectors[zone_type].sort([&](s32 sectorIndexA, s32 sectorIndexB) {
            return layer->sectors[sectorIndexA].averageDesirability[zone_type] > layer->sectors[sectorIndexB].averageDesirability[zone_type];
        });

// TEST
#if 0
		logInfo("Most desirable sectors ({0})", {zoneDefs[zoneType].name});
		for (s32 position = 0; position < getSectorCount(&layer->sectors); position++)
		{
			s32 sectorIndex = layer->mostDesirableSectors[zoneType][position];
			f32 averageDesirability = layer->sectors[sectorIndex].averageDesirability[zoneType];
			logInfo("#{0}: sector {1}, desirability {2}", {formatInt(position), formatInt(sectorIndex), formatFloat(averageDesirability, 3)});
		}
#endif
    }

    calculateDemand(city, layer);
    growSomeZoneBuildings(city);
}

void calculateDemand(City* city, ZoneLayer* layer)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    // Ratio of residents to job should be roughly 3 : 1

    // TODO: We want to consider AVAILABLE jobs/residents, not TOTAL ones.
    // TODO: This is a generally terrible calculation!

    s32 totalResidents = getTotalResidents(city);
    s32 totalJobs = getTotalJobs(city);

    // Residential
    layer->demand[ZoneType::Residential] = (totalJobs * 3) - totalResidents + 100;

    s32 totalJobsNeeded = (totalResidents / 3);

    // Commercial
    layer->demand[ZoneType::Commercial] = floor_s32(totalJobsNeeded * 0.2f) - layer->population[ZoneType::Commercial] + 20;

    // Industrial
    layer->demand[ZoneType::Industrial] = floor_s32(totalJobsNeeded * 0.8f) - layer->population[ZoneType::Industrial] + 50;
}

bool isZoneAcceptable(City* city, ZoneType zoneType, s32 x, s32 y)
{
    DEBUG_FUNCTION();

    ZoneDef const& def = ZONE_DEFS[zoneType];

    if (getZoneAt(city, x, y) != zoneType)
        return false;
    if (buildingExistsAt(city, x, y))
        return false;
    if (getDistanceToTransport(city, x, y, TransportType::Road) > def.maximumDistanceToRoad)
        return false;

    // TODO: Power requirements!

    return true;
}

template<typename Filter>
static BuildingDef* findRandomZoneBuilding(ZoneType zoneType, Random* random, Filter filter)
{
    DEBUG_FUNCTION();

    // Choose a random building, then carry on checking buildings until one is acceptable
    ChunkedArray<BuildingDef*>* buildings = nullptr;
    switch (zoneType) {
    case ZoneType::Residential:
        buildings = &buildingCatalogue.rGrowableBuildings;
        break;
    case ZoneType::Commercial:
        buildings = &buildingCatalogue.cGrowableBuildings;
        break;
    case ZoneType::Industrial:
        buildings = &buildingCatalogue.iGrowableBuildings;
        break;

        INVALID_DEFAULT_CASE;
    }

    BuildingDef* result = nullptr;

    // TODO: @RandomIterate - This random selection is biased, and wants replacing with an iteration only over valid options,
    // like in "growSomeZoneBuildings - find a valid zone".
    // Well, it does if growing buildings one at a time is how we want to do things. I'm not sure.
    // Growing a whole "block" of a building might make more sense for residential at least.
    // Something to decide on later.
    // - Sam, 18/08/2019
    for (auto it = buildings->iterate(randomBelow(random, truncate32(buildings->count)));
        it.hasNext();
        it.next()) {
        BuildingDef* def = it.getValue();

        if (filter(def)) {
            result = def;
            break;
        }
    }

    return result;
}

void growSomeZoneBuildings(City* city)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    ZoneLayer* layer = &city->zoneLayer;
    Random* random = &AppState::the().gameState->gameRandom;

    for (auto zone_type : enum_values<ZoneType>()) {
        if (layer->demand[zone_type] > 0) {
            s32 remainingBuildingCount = 8; // How many buildings can be constructed at once TODO: Tweak this number!
            s32 remainingDemand = layer->demand[zone_type];
            s32 minimumDemand = layer->demand[zone_type] / 20; // Stop when we're below 20% of the original demand

            s32 maxRBuildingDim = getMaxBuildingSize(zone_type);
            s32 savedPositionInMostDesirableSectorsTable = 0;

            while ((remainingBuildingCount > 0)
                && (layer->sectorsWithEmptyZones[zone_type].setBitCount > 0)
                && (remainingDemand > minimumDemand)) {
                bool foundAZone = false;
                s32 randomXOffset = randomNext(random);
                s32 randomYOffset = randomNext(random);

                V2I zonePos = {};
                {
                    DEBUG_BLOCK_T("growSomeZoneBuildings - find a valid zone", DebugCodeDataTag::Simulation);

                    // Go through sectors from most desirable to least
                    for (s32 position = savedPositionInMostDesirableSectorsTable; position < getSectorCount(&layer->sectors); position++) {
                        s32 sectorIndex = layer->mostDesirableSectors[zone_type][position];

                        // Skip if it doesn't have any available zones
                        // TODO: We want to penalise redevelopment, but still allow it
                        if (!layer->sectorsWithEmptyZones[zone_type][sectorIndex])
                            continue;

                        ZoneSector* sector = &layer->sectors.sectors[sectorIndex];

                        for (s32 relY = 0;
                            !foundAZone && relY < sector->bounds.h;
                            relY++) {
                            for (s32 relX = 0;
                                !foundAZone && relX < sector->bounds.w;
                                relX++) {
                                s32 tileX = (relX + randomXOffset) % sector->bounds.w;
                                s32 tileY = (relY + randomYOffset) % sector->bounds.h;
                                s32 x = sector->bounds.x + tileX;
                                s32 y = sector->bounds.y + tileY;

                                if (isZoneAcceptable(city, zone_type, x, y)) {
                                    zonePos = v2i(x, y);
                                    foundAZone = true;

                                    //
                                    // NB: We store this for two reasons:
                                    // 1) It saves us re-checking the same sectors that we know don't have available
                                    //    zones in.
                                    // 2) More importantly, if we don't, then we only ever try to build in the first
                                    //    valid sector! For example, if there's a 1x1 space in the best sector, but
                                    //    all of the possible buildings are larger than that, then we were just trying
                                    //    to fit stuff into that 1x1 space over and over, and never looking at less
                                    //    desirable sectors! Oops. Fixed now though.
                                    //
                                    // - Sam, 23/10/2019
                                    //
                                    savedPositionInMostDesirableSectorsTable = position;
                                }
                            }
                        }
                    }
                }

                if (!foundAZone) {
                    // There are empty zones, but none meet our requirements, so stop trying to grow anything
                    break;
                }

                // Produce a rectangle of the contiguous empty zone area around that point,
                // so we can fit larger buildings there if possible.
                Rect2I zoneFootprint = irectXYWH(zonePos.x, zonePos.y, 1, 1);
                {
                    DEBUG_BLOCK_T("growSomeZoneBuildings - expand rect", DebugCodeDataTag::Simulation);
                    // Gradually expand from zonePos outwards, checking if there is available, zoned space

                    bool tryX = randomBool(random);
                    bool positive = randomBool(random);

                    while (true) {
                        bool canExpand = true;
                        if (tryX) {
                            s32 x = positive ? (zoneFootprint.x + zoneFootprint.w) : (zoneFootprint.x - 1);

                            for (s32 y = zoneFootprint.y; y < zoneFootprint.y + zoneFootprint.h; y++) {
                                if (!isZoneAcceptable(city, zone_type, x, y)) {
                                    canExpand = false;
                                    break;
                                }
                            }

                            if (!canExpand) {
                                // Try again in the opposite direction
                                canExpand = true;
                                positive = !positive;

                                x = positive ? (zoneFootprint.x + zoneFootprint.w) : (zoneFootprint.x - 1);

                                for (s32 y = zoneFootprint.y; y < zoneFootprint.y + zoneFootprint.h; y++) {
                                    if (!isZoneAcceptable(city, zone_type, x, y)) {
                                        canExpand = false;
                                        break;
                                    }
                                }
                            }

                            if (canExpand) {
                                zoneFootprint.w++;
                                if (!positive)
                                    zoneFootprint.x--;
                            }
                        } else // expand in y
                        {
                            s32 y = positive ? (zoneFootprint.y + zoneFootprint.h) : (zoneFootprint.y - 1);

                            for (s32 x = zoneFootprint.x; x < zoneFootprint.x + zoneFootprint.w; x++) {
                                if (!isZoneAcceptable(city, zone_type, x, y)) {
                                    canExpand = false;
                                    break;
                                }
                            }

                            if (!canExpand) {
                                // Try again in the opposite direction
                                canExpand = true;
                                positive = !positive;

                                y = positive ? (zoneFootprint.y + zoneFootprint.h) : (zoneFootprint.y - 1);

                                for (s32 x = zoneFootprint.x; x < zoneFootprint.x + zoneFootprint.w; x++) {
                                    if (!isZoneAcceptable(city, zone_type, x, y)) {
                                        canExpand = false;
                                        break;
                                    }
                                }
                            }

                            if (canExpand) {
                                zoneFootprint.h++;
                                if (!positive)
                                    zoneFootprint.y--;
                            }
                        }

                        // As soon as we fail to expand, we stop.
                        // Maybe we could wait until we fail in both directions?
                        if (!canExpand)
                            break;

                        // No need to grow bigger than the largest possible building!
                        // (Unless at some point we do "batches" of buildings like SC4 does)
                        if (zoneFootprint.w >= maxRBuildingDim && zoneFootprint.h >= maxRBuildingDim)
                            break;

                        tryX = !tryX;                  // Alternate x and y
                        positive = randomBool(random); // random direction to expand in
                    }
                }

                // Pick a building def that fits the space and is not more than 10% more than the remaining demand
                s32 maxPopulation = (s32)((f32)remainingDemand * 1.1f);
                BuildingDef* buildingDef = findRandomZoneBuilding(zone_type, random, [=](BuildingDef* it) -> bool {
                    if ((it->width > zoneFootprint.w) || (it->height > zoneFootprint.h))
                        return false;

                    if (it->growsInZone == ZoneType::Residential) {
                        return (it->residents > 0) && (it->residents <= maxPopulation);
                    } else {
                        return (it->jobs > 0) && (it->jobs <= maxPopulation);
                    }
                });

                if (buildingDef) {
                    // Place it!
                    // TODO: This picks a random spot within the zoneFootprint; we should probably pick the most desirable part? @Desirability
                    Rect2I footprint = randomlyPlaceRectangle(random, buildingDef->size, zoneFootprint);

                    Building* building = addBuilding(city, buildingDef, footprint);
                    layer->population[zone_type] += building->currentResidents + building->currentJobs;
                    updateBuildingVariant(city, building, buildingDef);

                    markAreaDirty(city, footprint);

                    remainingDemand -= (building->currentResidents + building->currentJobs);
                    remainingBuildingCount--;
                } else {
                    savedPositionInMostDesirableSectorsTable++; // Go to the next sector in the hope it'll be better.

                    // // We failed to find a building def, so we should probably stop trying to build things!
                    // // Otherwise, we'll get stuck in an infinite loop
                    // break;
                }
            }
        }
    }
}

s32 getTotalResidents(City* city)
{
    return city->zoneLayer.population[ZoneType::Residential];
}

s32 getTotalJobs(City* city)
{
    return city->zoneLayer.population[ZoneType::Commercial] + city->zoneLayer.population[ZoneType::Industrial] + city->zoneLayer.population[ZoneType::None];
}

void saveZoneLayer(ZoneLayer* layer, BinaryFileWriter* writer)
{
    writer->startSection<SAVSection_Zone>(SAV_ZONE_ID, SAV_ZONE_VERSION);
    SAVSection_Zone zoneSection = {};

    // Tile zones
    zoneSection.tileZone = writer->appendBlob(layer->tileZone, FileBlobCompressionScheme::RLE_S8);

    writer->endSection<SAVSection_Zone>(&zoneSection);
}

bool loadZoneLayer(ZoneLayer* layer, City* /*city*/, BinaryFileReader* reader)
{
    bool succeeded = false;
    while (reader->startSection(SAV_ZONE_ID, SAV_ZONE_VERSION)) {
        SAVSection_Zone* section = reader->readStruct<SAVSection_Zone>(0);
        if (!section)
            break;

        if (!reader->readBlob(section->tileZone, &layer->tileZone))
            break;

        succeeded = true;
        break;
    }

    return succeeded;
}
