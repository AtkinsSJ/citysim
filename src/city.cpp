/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "city.h"
#include "AppState.h"
#include "Assets/AssetManager.h"
#include "Util/Random.h"
#include "Util/Rectangle.h"
#include "binary_file_reader.h"
#include "binary_file_writer.h"
#include "save_file.h"
#include "write_buffer.h"
#include <Gfx/Renderer.h>

void initCity(MemoryArena* gameArena, City* city, u32 width, u32 height, String name, String playerName, s32 funds)
{
    *city = {};

    // TODO: These want to be in some kind of buffer somewhere so they can be modified!
    city->name = pushString(gameArena, name);
    city->playerName = pushString(gameArena, playerName);
    city->funds = funds;
    city->bounds = irectXYWH(0, 0, width, height);

    city->tileBuildingIndex = gameArena->allocate_array_2d<s32>(width, height);
    initChunkPool(&city->sectorBuildingsChunkPool, gameArena, 128);
    initChunkPool(&city->sectorBoundariesChunkPool, gameArena, 8);
    initChunkPool(&city->buildingRefsChunkPool, gameArena, 128);

    initSectorGrid(&city->sectors, gameArena, width, height, 16, 8);
    for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&city->sectors); sectorIndex++) {
        CitySector* sector = &city->sectors.sectors[sectorIndex];
        initChunkedArray(&sector->ownedBuildings, &city->sectorBuildingsChunkPool);
    }

    initOccupancyArray(&city->buildings, gameArena, 1024);
    city->buildings.append(); // Null building

    initOccupancyArray(&city->entities, gameArena, 1024);

    initBudgetLayer(&city->budgetLayer, city, gameArena);
    initCrimeLayer(&city->crimeLayer, city, gameArena);
    initEducationLayer(&city->educationLayer, city, gameArena);
    initFireLayer(&city->fireLayer, city, gameArena);
    initHealthLayer(&city->healthLayer, city, gameArena);
    initLandValueLayer(&city->landValueLayer, city, gameArena);
    initPollutionLayer(&city->pollutionLayer, city, gameArena);
    initPowerLayer(&city->powerLayer, city, gameArena);
    initTerrainLayer(&city->terrainLayer, city, gameArena);
    initTransportLayer(&city->transportLayer, city, gameArena);
    initZoneLayer(&city->zoneLayer, city, gameArena);

    city->highestBuildingID = 0;

    // TODO: The rest of this code doesn't really belong here!
    // It belongs in a "we've just started/loaded a game, so initialise things" place.

    // TODO: Are we sure we want to do this?
    markAreaDirty(city, city->bounds);

    the_renderer().world_camera().set_position(v2(city->bounds.size) / 2);

    saveBuildingTypes();
    saveTerrainTypes();
}

void removeEntity(City* city, Entity* entity)
{
    // logInfo("Removing entity #{0}"_s, {formatInt(entity->index)});
    city->entities.removeIndex(entity->index);
}

void drawEntities(City* city, Rect2I visibleTileBounds)
{
    // TODO: Depth sorting
    // TODO: Sectors maybe? Though collecting all the visible entities into a data structure might be slower than just
    //  iterating the entities array.
    Rect2 cropArea = rect2(visibleTileBounds);
    auto shaderID = the_renderer().shaderIds.pixelArt;

    bool isDemolitionHappening = (areaOf(city->demolitionRect) > 0);
    auto drawColorDemolish = Colour::from_rgb_255(255, 128, 128, 255);
    Rect2 demolitionRect = rect2(city->demolitionRect);

    for (auto it = city->entities.iterate(); it.hasNext(); it.next()) {
        auto entity = it.get();
        if (overlaps(cropArea, entity->bounds)) {
            // TODO: Batch these together somehow? Our batching is a bit complicated.
            // OK, turns out our renderer still batches same-texture-same-shader calls into a single draw call!
            // So, the difference is just sending N x RenderItem_DrawSingleRect instead of 1 x RenderItem_DrawRects
            // Thanks, past me!
            // - Sam, 26/09/2020

            auto drawColor = entity->color;

            if (entity->canBeDemolished && isDemolitionHappening && overlaps(entity->bounds, demolitionRect)) {
                drawColor = drawColor.multiplied_by(drawColorDemolish);
            }

            drawSingleSprite(&the_renderer().world_buffer(), getSprite(&entity->sprite), entity->bounds, shaderID, drawColor);
        }
    }
}

Building* addBuildingDirect(City* city, s32 id, BuildingDef* def, Rect2I footprint, GameTimestamp creationDate)
{
    DEBUG_FUNCTION();

    Indexed<Building*> buildingSlot = city->buildings.append();
    s32 buildingIndex = buildingSlot.index;
    Building* building = buildingSlot.value;
    initBuilding(building, id, def, footprint, creationDate);

    // Random sprite!
    building->spriteOffset = randomNext(&AppState::the().cosmeticRandom);

    building->entity = addEntity(city, Entity::Type::Building, building);
    building->entity->bounds = rect2(footprint);
    loadBuildingSprite(building);
    building->entity->canBeDemolished = true;

    CitySector* ownerSector = getSectorAtTilePos(&city->sectors, footprint.x, footprint.y);
    ownerSector->ownedBuildings.append(building);

    for (s32 y = footprint.y;
        y < footprint.y + footprint.h;
        y++) {
        for (s32 x = footprint.x;
            x < footprint.x + footprint.w;
            x++) {
            city->tileBuildingIndex.set(x, y, buildingIndex);
        }
    }

    notifyNewBuilding(&city->crimeLayer, def, building);
    notifyNewBuilding(&city->fireLayer, def, building);
    notifyNewBuilding(&city->healthLayer, def, building);
    notifyNewBuilding(&city->powerLayer, def, building);

    return building;
}

Building* addBuilding(City* city, BuildingDef* def, Rect2I footprint, GameTimestamp creationDate)
{
    DEBUG_FUNCTION();

    Building* building = addBuildingDirect(city, ++city->highestBuildingID, def, footprint, creationDate);

    // TODO: Properly calculate occupancy!
    building->currentResidents = def->residents;
    building->currentJobs = def->jobs;

    return building;
}

void markAreaDirty(City* city, Rect2I bounds)
{
    markHealthLayerDirty(&city->healthLayer, bounds);
    markLandValueLayerDirty(&city->landValueLayer, bounds);
    markPollutionLayerDirty(&city->pollutionLayer, bounds);
    markPowerLayerDirty(&city->powerLayer, bounds);
    markTransportLayerDirty(&city->transportLayer, bounds);
}

bool tileExists(City* city, s32 x, s32 y)
{
    return (x >= 0) && (x < city->bounds.w)
        && (y >= 0) && (y < city->bounds.h);
}

bool canAfford(City* city, s32 cost)
{
    return city->funds >= cost;
}

void spend(City* city, s32 cost)
{
    city->funds -= cost;
}

bool canPlaceBuilding(City* city, BuildingDef* def, s32 left, s32 top)
{
    DEBUG_FUNCTION();

    // Can we afford to build this?
    if (!canAfford(city, def->buildCost)) {
        return false;
    }

    Rect2I footprint = irectXYWH(left, top, def->width, def->height);

    // Are we in bounds?
    if (!contains(irectXYWH(0, 0, city->bounds.w, city->bounds.h), footprint)) {
        return false;
    }

    // Check terrain is buildable and empty
    // TODO: Optimise this per-sector!
    for (s32 y = footprint.y; y < footprint.y + footprint.h; y++) {
        for (s32 x = footprint.x; x < footprint.x + footprint.w; x++) {
            TerrainDef* terrainDef = getTerrainAt(city, x, y);

            if (!terrainDef->canBuildOn) {
                return false;
            }

            Building* buildingAtPos = getBuildingAt(city, x, y);
            if (buildingAtPos != nullptr) {
                // Check if we can combine this with the building that's already there
                Maybe<BuildingDef*> possibleIntersection = findBuildingIntersection(getBuildingDef(buildingAtPos->typeID), def);
                if (possibleIntersection.isValid) {
                    // We can!
                    // TODO: We want to check if there is a valid variant, before we build.
                    // But that means matching against buildings that aren't constructed yet,
                    // but just part of the drag-rect planned construction!
                    // (eg, dragging a road across a rail, we want to check for a rail-crossing
                    // variant that matches the planned road tiles.)
                } else {
                    return false;
                }
            }
        }
    }
    return true;
}

void placeBuilding(City* city, BuildingDef* def, s32 left, s32 top, bool markAreasDirty)
{
    DEBUG_FUNCTION();

    Rect2I footprint = irectXYWH(left, top, def->width, def->height);

    Building* building = getBuildingAt(city, left, top);
    if (building != nullptr) {
        // Do a quick replace! We already established in canPlaceBuilding() that we match.
        // NB: We're keeping the old building's id. I think that's preferable, but might want to change that later.
        BuildingDef* oldDef = getBuildingDef(building->typeID);

        Maybe<BuildingDef*> intersectionDef = findBuildingIntersection(oldDef, def);
        ASSERT(intersectionDef.isValid);

        building->typeID = intersectionDef.value->typeID;
        def = intersectionDef.value; // I really don't like this but I don't want to rewrite this entire function right now!

        city->zoneLayer.population[oldDef->growsInZone] -= building->currentResidents + building->currentJobs;
    } else {
        // Remove zones
        placeZone(city, ZoneType::None, footprint);

        building = addBuilding(city, def, footprint);
    }

    // TODO: Calculate residents/jobs properly!
    building->currentResidents = def->residents;
    building->currentJobs = def->jobs;

    city->zoneLayer.population[def->growsInZone] += building->currentResidents + building->currentJobs;

    updateBuildingVariant(city, building, def);
    updateAdjacentBuildingVariants(city, footprint);

    if (markAreasDirty) {
        markAreaDirty(city, footprint);
    }
}

s32 calculateBuildCost(City* city, BuildingDef* def, Rect2I area)
{
    DEBUG_FUNCTION();

    s32 totalCost = 0;

    for (s32 y = 0; y + def->height <= area.h; y += def->height) {
        for (s32 x = 0; x + def->width <= area.w; x += def->width) {
            if (canPlaceBuilding(city, def, area.x + x, area.y + y)) {
                totalCost += def->buildCost;
            }
        }
    }

    return totalCost;
}

void placeBuildingRect(City* city, BuildingDef* def, Rect2I area)
{
    DEBUG_FUNCTION();

    for (s32 y = 0; y + def->height <= area.h; y += def->height) {
        for (s32 x = 0; x + def->width <= area.w; x += def->width) {
            if (canPlaceBuilding(city, def, area.x + x, area.y + y)) {
                placeBuilding(city, def, area.x + x, area.y + y, false);
            }
        }
    }

    markAreaDirty(city, area);
}

s32 calculateDemolitionCost(City* city, Rect2I area)
{
    DEBUG_FUNCTION();

    s32 total = 0;

    // Building demolition cost
    ChunkedArray<Building*> buildingsToDemolish = findBuildingsOverlappingArea(city, area);
    for (auto it = buildingsToDemolish.iterate(); it.hasNext(); it.next()) {
        Building* building = it.getValue();
        total += getBuildingDef(building->typeID)->demolishCost;
    }

    return total;
}

void demolishRect(City* city, Rect2I area)
{
    DEBUG_FUNCTION();

    // NB: We assume that we've already checked we can afford this!

    // Building demolition
    ChunkedArray<Building*> buildingsToDemolish = findBuildingsOverlappingArea(city, area);
    for (auto it = buildingsToDemolish.iterate(buildingsToDemolish.count - 1, false, true);
        it.hasNext();
        it.next()) {
        Building* building = it.getValue();
        BuildingDef* def = getBuildingDef(building->typeID);

        city->zoneLayer.population[def->growsInZone] -= building->currentResidents + building->currentJobs;

        Rect2I buildingFootprint = building->footprint;

        // Clean up other references
        notifyBuildingDemolished(&city->crimeLayer, def, building);
        notifyBuildingDemolished(&city->fireLayer, def, building);
        notifyBuildingDemolished(&city->healthLayer, def, building);
        notifyBuildingDemolished(&city->powerLayer, def, building);

        building->id = 0;
        building->typeID = -1;

        s32 buildingIndex = city->tileBuildingIndex.get(buildingFootprint.x, buildingFootprint.y);
        city->buildings.removeIndex(buildingIndex);
        removeEntity(city, building->entity);

        building = nullptr; // For safety, because we just deleted the Building!

        for (s32 y = buildingFootprint.y;
            y < buildingFootprint.y + buildingFootprint.h;
            y++) {
            for (s32 x = buildingFootprint.x;
                x < buildingFootprint.x + buildingFootprint.w;
                x++) {
                city->tileBuildingIndex.set(x, y, 0);
            }
        }

        // Only need to add the footprint as a separate rect if it's not inside the area!
        if (!contains(area, buildingFootprint)) {
            markZonesAsEmpty(city, buildingFootprint);
            markAreaDirty(city, buildingFootprint);
        }
    }

    // Expand the area to account for buildings to the left or up from it
    Rect2I expandedArea = expand(area, buildingCatalogue.overallMaxBuildingDim, 0, 0, buildingCatalogue.overallMaxBuildingDim);
    Rect2I sectorsArea = getSectorsCovered(&city->sectors, expandedArea);

    for (s32 sY = sectorsArea.y;
        sY < sectorsArea.y + sectorsArea.h;
        sY++) {
        for (s32 sX = sectorsArea.x;
            sX < sectorsArea.x + sectorsArea.w;
            sX++) {
            CitySector* sector = getSector(&city->sectors, sX, sY);

            // Rebuild the ownedBuildings array
            sector->ownedBuildings.clear();

            for (s32 y = sector->bounds.y;
                y < sector->bounds.y + sector->bounds.h;
                y++) {
                for (s32 x = sector->bounds.x;
                    x < sector->bounds.x + sector->bounds.w;
                    x++) {
                    Building* b = getBuildingAt(city, x, y);
                    if (b != nullptr) {
                        if (b->footprint.x == x && b->footprint.y == y) {
                            sector->ownedBuildings.append(b);
                        }
                    }
                }
            }
        }
    }

    // Mark area as changed
    markZonesAsEmpty(city, area);
    markAreaDirty(city, area);

    // Any buildings that would have connected with something that just got demolished need to refresh!
    updateAdjacentBuildingVariants(city, area);
}

ChunkedArray<Building*> findBuildingsOverlappingArea(City* city, Rect2I area, Flags<BuildingQueryFlag> flags)
{
    DEBUG_FUNCTION();

    ChunkedArray<Building*> result = {};
    initChunkedArray(&result, &temp_arena(), 64);

    // Expand the area to account for buildings to the left or up from it
    // (but don't do that if we only care about origins)
    s32 expansion = flags.has(BuildingQueryFlag::RequireOriginInArea) ? 0 : buildingCatalogue.overallMaxBuildingDim;
    Rect2I expandedArea = expand(area, expansion, 0, 0, expansion);
    Rect2I sectorsArea = getSectorsCovered(&city->sectors, expandedArea);

    for (s32 sY = sectorsArea.y;
        sY < sectorsArea.y + sectorsArea.h;
        sY++) {
        for (s32 sX = sectorsArea.x;
            sX < sectorsArea.x + sectorsArea.w;
            sX++) {
            CitySector* sector = getSector(&city->sectors, sX, sY);

            for (auto it = sector->ownedBuildings.iterate(); it.hasNext(); it.next()) {
                Building* building = it.getValue();
                if (overlaps(building->footprint, area)) {
                    result.append(building);
                }
            }
        }
    }

    return result;
}

void drawCity(City* city, Rect2I visibleTileBounds)
{
    auto& renderer = the_renderer();
    drawTerrain(city, visibleTileBounds, renderer.shaderIds.pixelArt);

    drawZones(city, visibleTileBounds, renderer.shaderIds.untextured);

    // drawBuildings(city, visibleTileBounds, renderer.shaderIds.pixelArt, demolitionRect);

    drawEntities(city, visibleTileBounds);

    // Draw sectors
    // NB: this is really hacky debug code
    if (false) {
        Rect2I visibleSectors = getSectorsCovered(&city->sectors, visibleTileBounds);
        DrawRectsGroup* group = beginRectsGroupUntextured(&renderer.world_overlay_buffer(), renderer.shaderIds.untextured, areaOf(visibleSectors));
        auto sectorColor = Colour::from_rgb_255(255, 255, 255, 40);
        for (s32 sy = visibleSectors.y;
            sy < visibleSectors.y + visibleSectors.h;
            sy++) {
            for (s32 sx = visibleSectors.x;
                sx < visibleSectors.x + visibleSectors.w;
                sx++) {
                if ((sx + sy) % 2) {
                    CitySector* sector = getSector(&city->sectors, sx, sy);
                    addUntexturedRect(group, rect2(sector->bounds), sectorColor);
                }
            }
        }
        endRectsGroup(group);
    }
}

bool buildingExistsAt(City* city, s32 x, s32 y)
{
    bool result = city->tileBuildingIndex.get(x, y) > 0;

    return result;
}

Building* getBuildingAt(City* city, s32 x, s32 y)
{
    Building* result = nullptr;

    if (tileExists(city, x, y)) {
        u32 buildingID = city->tileBuildingIndex.get(x, y);
        if (buildingID > 0) {
            result = city->buildings.get(buildingID);
        }
    }

    return result;
}

// Runs an update on X sectors' buildings, gradually covering the whole city with subsequent calls.
void updateSomeBuildings(City* city)
{
    for (s32 i = 0; i < city->sectors.sectorsToUpdatePerTick; i++) {
        CitySector* sector = getNextSector(&city->sectors);

        for (auto it = sector->ownedBuildings.iterate(); it.hasNext(); it.next()) {
            Building* building = it.getValue();
            updateBuilding(city, building);
        }
    }
}

void saveBuildings(City* city, struct BinaryFileWriter* writer)
{
    writer->startSection<SAVSection_Buildings>(SAV_BUILDING_ID, SAV_BUILDING_VERSION);
    SAVSection_Buildings buildingSection = {};

    // Building types table
    s32 buildingDefCount = buildingCatalogue.allBuildings.count - 1; // Skip the null def
    WriteBufferRange buildingTypeTableLoc = writer->reserveArray<SAVBuildingTypeEntry>(buildingDefCount);
    Array<SAVBuildingTypeEntry> buildingTypeTable = writer->arena->allocate_array<SAVBuildingTypeEntry>(buildingDefCount);
    for (auto it = buildingCatalogue.allBuildings.iterate(); it.hasNext(); it.next()) {
        BuildingDef* def = it.get();
        if (def->typeID == 0)
            continue; // Skip the null building def!

        SAVBuildingTypeEntry* entry = buildingTypeTable.append();
        entry->typeID = def->typeID;
        entry->name = writer->appendString(def->name);
    }
    buildingSection.buildingTypeTable = writer->writeArray<SAVBuildingTypeEntry>(buildingTypeTable, buildingTypeTableLoc);

    // Highest ID
    buildingSection.highestBuildingID = city->highestBuildingID;

    //
    // The buildings themselves!
    // I'm not sure how to actually do this... current thought is that the "core" building
    // data will be here, and that other stuff (eg, health/education of the occupants)
    // will be in the relevant layers' chunks. That seems the most extensible?
    // Another option would be to store the buildings here as a series of arrays, one
    // for each field, like we do for Terrain. That feels really messy though.
    // The tricky thing is that the Building struct in game feels likely to change a lot,
    // and we want the save format to change as little as possible... though that only
    // matters once the game is released (or near enough release that I start making
    // pre-made maps) so maybe it's not such a big issue?
    // Eh, going to just go ahead with a placeholder version, like the rest of this code!
    //
    // - Sam, 11/10/2019
    //
    buildingSection.buildingCount = city->buildings.count - 1; // Not the null building!

    SAVBuilding* tempBuildings = writer->arena->allocate_multiple<SAVBuilding>(buildingSection.buildingCount);
    s32 tempBuildingIndex = 0;

    for (auto it = city->buildings.iterate(); it.hasNext(); it.next()) {
        Building* building = it.get();
        if (building->id == 0)
            continue; // Skip the null building!

        SAVBuilding* sb = tempBuildings + tempBuildingIndex;
        *sb = {};
        sb->id = building->id;
        sb->typeID = building->typeID;
        sb->creationDate = building->creationDate;
        sb->x = (u16)building->footprint.x;
        sb->y = (u16)building->footprint.y;
        sb->w = (u16)building->footprint.w;
        sb->h = (u16)building->footprint.h;
        sb->spriteOffset = (u16)building->spriteOffset;
        sb->currentResidents = (u16)building->currentResidents;
        sb->currentJobs = (u16)building->currentJobs;
        sb->variantIndex = static_cast<s16>(building->variantIndex.value_or(-1));

        tempBuildingIndex++;
    }
    buildingSection.buildings = writer->appendBlob(buildingSection.buildingCount * sizeof(SAVBuilding), (u8*)tempBuildings, FileBlobCompressionScheme::Uncompressed);

    writer->endSection<SAVSection_Buildings>(&buildingSection);
}

bool loadBuildings(City* city, struct BinaryFileReader* reader)
{
    bool succeeded = reader->startSection(SAV_BUILDING_ID, SAV_BUILDING_VERSION);
    while (succeeded) {
        SAVSection_Buildings* section = reader->readStruct<SAVSection_Buildings>(0);
        if (!section)
            break;

        city->highestBuildingID = section->highestBuildingID;

        // Map the file's building type IDs to the game's ones
        // NB: count+1 because the file won't save the null building, so we need to compensate
        Array<u32> oldTypeToNewType = reader->arena->allocate_array<u32>(section->buildingTypeTable.count + 1, true);
        Array<SAVBuildingTypeEntry> buildingTypeTable = reader->arena->allocate_array<SAVBuildingTypeEntry>(section->buildingTypeTable.count);
        if (!reader->readArray(section->buildingTypeTable, &buildingTypeTable))
            break;
        for (s32 i = 0; i < buildingTypeTable.count; i++) {
            SAVBuildingTypeEntry* entry = &buildingTypeTable[i];
            String buildingName = reader->readString(entry->name);

            BuildingDef* def = findBuildingDef(buildingName);
            if (def == nullptr) {
                // The building doesn't exist in the game... we'll remap to 0
                //
                // Ideally, we'd keep the information about what a building really is, so
                // that if it's later loaded into a game that does have that building, it'll work
                // again instead of being forever lost. But, we're talking about a situation of
                // the player messing around with the data files, or adding/removing/adding mods,
                // so IDK. The saved game is already corrupted if you load it and stuff is missing -
                // playing at all from that point is going to break things, and if we later make
                // that stuff appear again, we're actually just breaking it a second time!
                //
                // So maybe, the real "correct" solution is to tell the player that things are missing
                // from the game, and then just demolish the missing-id buildings.
                //
                // Mods probably want some way to define "migrations" for transforming saved games
                // when the mod's data changes. That's probably a good idea for the base game's data
                // too, because changes happen!
                //
                // Lots to think about!
                //
                // - Sam, 11/11/2019
                //
                oldTypeToNewType[entry->typeID] = 0;
            } else {
                oldTypeToNewType[entry->typeID] = def->typeID;
            }
        }

        Array<SAVBuilding> tempBuildings = reader->arena->allocate_array<SAVBuilding>(section->buildingCount);
        if (!reader->readBlob(section->buildings, &tempBuildings))
            break;
        for (u32 buildingIndex = 0;
            buildingIndex < section->buildingCount;
            buildingIndex++) {
            SAVBuilding* savBuilding = &tempBuildings[buildingIndex];

            Rect2I footprint = irectXYWH(savBuilding->x, savBuilding->y, savBuilding->w, savBuilding->h);
            BuildingDef* def = getBuildingDef(oldTypeToNewType[savBuilding->typeID]);
            Building* building = addBuildingDirect(city, savBuilding->id, def, footprint, savBuilding->creationDate);
            building->variantIndex = savBuilding->variantIndex == -1 ? Optional<s16> {} : Optional<s16> { savBuilding->variantIndex.value() };
            building->spriteOffset = savBuilding->spriteOffset;
            building->currentResidents = savBuilding->currentResidents;
            building->currentJobs = savBuilding->currentJobs;
            // Because the sprite was assigned in addBuildingDirect(), before we loaded the variant, we
            // need to overwrite the sprite here.
            // Probably there's a better way to organise this, but this works.
            // - Sam, 26/09/2020
            loadBuildingSprite(building);

            // This is a bit hacky but it's how we calculate it elsewhere
            city->zoneLayer.population[def->growsInZone] += building->currentResidents + building->currentJobs;
        }

        break;
    }

    return succeeded;
}
