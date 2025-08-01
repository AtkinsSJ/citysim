/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Power.h"
#include "../save_file.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Sim/City.h>
#include <UI/Panel.h>
#include <Util/Set.h>

void initPowerLayer(PowerLayer* layer, City* city, MemoryArena* gameArena)
{
    initChunkedArray(&layer->networks, gameArena, 64);
    initChunkPool(&layer->powerGroupsChunkPool, gameArena, 4);
    initChunkPool(&layer->powerGroupPointersChunkPool, gameArena, 32);

    layer->tilePowerDistance = gameArena->allocate_array_2d<u8>(city->bounds.size());
    fill<u8>(&layer->tilePowerDistance, 255);
    layer->powerMaxDistance = 2;
    initDirtyRects(&layer->dirtyRects, gameArena, layer->powerMaxDistance, city->bounds);

    initSectorGrid(&layer->sectors, gameArena, city->bounds.size(), 16);
    for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&layer->sectors); sectorIndex++) {
        PowerSector* sector = &layer->sectors.sectors[sectorIndex];

        sector->tilePowerGroup = gameArena->allocate_array_2d<u8>(sector->bounds.size());

        initChunkedArray(&sector->powerGroups, &layer->powerGroupsChunkPool);
    }

    initChunkedArray(&layer->powerBuildings, &city->buildingRefsChunkPool);
}

PowerNetwork* newPowerNetwork(PowerLayer* layer)
{
    PowerNetwork* network = layer->networks.appendBlank();
    network->id = (s32)layer->networks.count;
    initChunkedArray(&network->groups, &layer->powerGroupPointersChunkPool);

    return network;
}

void freePowerNetwork(PowerNetwork* network)
{
    network->id = 0;
    network->groups.clear();
}

u8 getPowerGroupID(PowerSector* sector, s32 relX, s32 relY)
{
    return sector->tilePowerGroup.get(relX, relY);
}

void setPowerGroupID(PowerSector* sector, s32 relX, s32 relY, u8 value)
{
    sector->tilePowerGroup.set(relX, relY, value);
}

PowerGroup* getPowerGroupAt(PowerSector* sector, s32 relX, s32 relY)
{
    PowerGroup* result = nullptr;

    if (sector != nullptr) {
        s32 powerGroupID = getPowerGroupID(sector, relX, relY);
        if (powerGroupID != 0 && powerGroupID != POWER_GROUP_UNKNOWN) {
            result = sector->powerGroups.get(powerGroupID - 1);
        }
    }

    return result;
}

u8 getDistanceToPower(City* city, s32 x, s32 y)
{
    return city->powerLayer.tilePowerDistance.get(x, y);
}

u8 calculatePowerOverlayForTile(City* city, s32 x, s32 y)
{
    u8 paletteIndexPowered = 0;
    u8 paletteIndexBrownout = 1;
    u8 paletteIndexBlackout = 2;
    u8 paletteIndexNone = 255;

    u8 result = paletteIndexNone;

    PowerNetwork* network = getPowerNetworkAt(city, x, y);
    if (network == nullptr) {
        result = paletteIndexNone;
    } else if (network->cachedProduction == 0) {
        result = paletteIndexBlackout;
    } else if (network->cachedProduction < network->cachedConsumption) {
        result = paletteIndexBrownout;
    } else {
        result = paletteIndexPowered;
    }

    return result;
}

void updateSectorPowerValues(City* city, PowerSector* sector)
{
    DEBUG_FUNCTION();

    // Reset each to 0
    for (auto it = sector->powerGroups.iterate();
        it.hasNext();
        it.next()) {
        PowerGroup* powerGroup = it.get();
        powerGroup->production = 0;
        powerGroup->consumption = 0;
    }

    // Count power from buildings
    ChunkedArray<Building*> sectorBuildings = findBuildingsOverlappingArea(city, sector->bounds, BuildingQueryFlag::RequireOriginInArea);
    for (auto it = sectorBuildings.iterate();
        it.hasNext();
        it.next()) {
        Building* building = it.getValue();
        BuildingDef* def = getBuildingDef(building->typeID);

        if (def->power != 0) {
            u8 powerGroupIndex = getPowerGroupID(sector, building->footprint.x - sector->bounds.x, building->footprint.y - sector->bounds.y);
            PowerGroup* powerGroup = sector->powerGroups.get(powerGroupIndex - 1);
            if (def->power > 0) {
                powerGroup->production += def->power;
            } else {
                powerGroup->consumption -= def->power;
            }
        }
    }
}

bool doesTileHavePowerNetwork(City* city, s32 x, s32 y)
{
    bool result = false;

    if (tileExists(city, x, y)) {
        PowerLayer* layer = &city->powerLayer;
        PowerSector* sector = getSectorAtTilePos(&layer->sectors, x, y);

        s32 relX = x - sector->bounds.x;
        s32 relY = y - sector->bounds.y;

        u8 powerGroupIndex = getPowerGroupID(sector, relX, relY);
        result = (powerGroupIndex != 0);
    }

    return result;
}

PowerNetwork* getPowerNetworkAt(City* city, s32 x, s32 y)
{
    PowerNetwork* result = nullptr;

    if (tileExists(city, x, y)) {
        PowerLayer* layer = &city->powerLayer;
        PowerSector* sector = getSectorAtTilePos(&layer->sectors, x, y);

        s32 relX = x - sector->bounds.x;
        s32 relY = y - sector->bounds.y;

        u8 powerGroupIndex = getPowerGroupID(sector, relX, relY);
        if (powerGroupIndex != 0) {
            PowerGroup* group = sector->powerGroups.get(powerGroupIndex - 1);
            result = layer->networks.get(group->networkID - 1);
        }
    }

    return result;
}

void floodFillSectorPowerGroup(PowerSector* sector, s32 x, s32 y, u8 fillValue)
{
    DEBUG_FUNCTION();

    // Theoretically, the only place we non-recursively call this is in recalculateSectorPowerGroups(),
    // where we go top-left to bottom-right, so we only need to flood fill right and down from the
    // initial point!
    // However, I'm not sure right now if we might want to flood fill starting from some other point,
    // so I'll leave this as is.
    // Actually... no, that's not correct, because we could have this situation:
    //
    // .................
    // .............@...
    // ...######....#...
    // ...#....######...
    // ...#.............
    // .................
    //
    // Starting at @, following the path of power lines (#) takes us left and up!
    // So, yeah. It's fine as is!
    //
    // - Sam, 10/06/2019
    //

    setPowerGroupID(sector, x, y, fillValue);

    if ((x > 0) && (getPowerGroupID(sector, x - 1, y) == POWER_GROUP_UNKNOWN)) {
        floodFillSectorPowerGroup(sector, x - 1, y, fillValue);
    }

    if ((x < sector->bounds.width() - 1) && (getPowerGroupID(sector, x + 1, y) == POWER_GROUP_UNKNOWN)) {
        floodFillSectorPowerGroup(sector, x + 1, y, fillValue);
    }

    if ((y > 0) && (getPowerGroupID(sector, x, y - 1) == POWER_GROUP_UNKNOWN)) {
        floodFillSectorPowerGroup(sector, x, y - 1, fillValue);
    }

    if ((y < sector->bounds.height() - 1) && (getPowerGroupID(sector, x, y + 1) == POWER_GROUP_UNKNOWN)) {
        floodFillSectorPowerGroup(sector, x, y + 1, fillValue);
    }
}

void setRectPowerGroupUnknown(PowerSector* sector, Rect2I area)
{
    DEBUG_FUNCTION();

    Rect2I relArea = sector->bounds.intersected_relative(area);

    for (s32 relY = relArea.y;
        relY < relArea.y + relArea.height();
        relY++) {
        for (s32 relX = relArea.x;
            relX < relArea.x + relArea.width();
            relX++) {
            setPowerGroupID(sector, relX, relY, POWER_GROUP_UNKNOWN);
        }
    }
}

void markPowerLayerDirty(PowerLayer* layer, Rect2I bounds)
{
    markRectAsDirty(&layer->dirtyRects, bounds);
}

void addBuildingToPowerLayer(PowerLayer* layer, Building* building)
{
    PowerSector* sector = getSectorAtTilePos(&layer->sectors, building->footprint.x, building->footprint.y);
    PowerGroup* group = getPowerGroupAt(sector, building->footprint.x - sector->bounds.x, building->footprint.y - sector->bounds.y);
    if (group != nullptr) {
        group->buildings.append(getReferenceTo(building));
    }
}

void recalculateSectorPowerGroups(City* city, PowerSector* sector)
{
    DEBUG_FUNCTION();

    PowerLayer* layer = &city->powerLayer;

    // TODO: Clear any references to the PowerGroups that the City itself might have!
    // (I don't know how that's going to be structured yet.)
    // Meaning, if a city-wide power network knows that PowerGroup 3 in this sector is part of it,
    // we need to tell it that PowerGroup 3 is being destroyed!

    // Step 0: Remove the old PowerGroups.
    for (auto it = sector->powerGroups.iterate(); it.hasNext(); it.next()) {
        PowerGroup* powerGroup = it.get();
        powerGroup->sectorBoundaries.clear();
    }
    sector->powerGroups.clear();
    fill<u8>(&sector->tilePowerGroup, 0);

    // Step 1: Set all power-carrying tiles to POWER_GROUP_UNKNOWN (everything was set to 0 in the above memset())
    for (s32 relY = 0;
        relY < sector->bounds.height();
        relY++) {
        for (s32 relX = 0;
            relX < sector->bounds.width();
            relX++) {
            u8 distanceToPower = layer->tilePowerDistance.get(relX + sector->bounds.x, relY + sector->bounds.y);

            if (distanceToPower <= 1) {
                setPowerGroupID(sector, relX, relY, POWER_GROUP_UNKNOWN);
            } else {
                setPowerGroupID(sector, relX, relY, 0);
            }
        }
    }

    // Step 2: Flood fill each -1 tile as a local PowerGroup
    for (s32 relY = 0;
        relY < sector->bounds.height();
        relY++) {
        for (s32 relX = 0;
            relX < sector->bounds.width();
            relX++) {
            // Skip tiles that have already been added to a PowerGroup
            if (getPowerGroupID(sector, relX, relY) != POWER_GROUP_UNKNOWN)
                continue;

            PowerGroup* newGroup = sector->powerGroups.appendBlank();

            initChunkedArray(&newGroup->sectorBoundaries, &city->sectorBoundariesChunkPool);
            initChunkedArray(&newGroup->buildings, &city->buildingRefsChunkPool);

            u8 powerGroupID = (u8)sector->powerGroups.count;
            newGroup->production = 0;
            newGroup->consumption = 0;
            floodFillSectorPowerGroup(sector, relX, relY, powerGroupID);
        }
    }

    // At this point, if there are no power groups we can just stop.
    if (sector->powerGroups.isEmpty())
        return;

    // Store references to the buildings in each group, for faster updating later
    ChunkedArray<Building*> sectorBuildings = findBuildingsOverlappingArea(city, sector->bounds);
    for (auto it = sectorBuildings.iterate();
        it.hasNext();
        it.next()) {
        Building* building = it.getValue();
        if (getBuildingDef(building->typeID)->power == 0)
            continue; // We only care about powered buildings!

        if (sector->bounds.contains(building->footprint.position())) {
            PowerGroup* group = getPowerGroupAt(sector, building->footprint.x - sector->bounds.x, building->footprint.y - sector->bounds.y);

            ASSERT(group != nullptr);
            group->buildings.append(getReferenceTo(building));
        }
    }

    // Step 3: Calculate power production/consumption for OWNED buildings, and add to their PowerGroups
    updateSectorPowerValues(city, sector);

    // Step 4: Find and store the PowerGroup boundaries along the sector's edges, on the OUTSIDE
    // @Copypasta The code for all this is really repetitive, but I'm not sure how to factor it together nicely.

    // - Step 4.1: Left edge
    if (sector->bounds.x > 0) {
        u8 currentPGId = 0;
        Rect2I* currentBoundary = nullptr;

        s32 relX = 0;
        for (s32 relY = 0;
            relY < sector->bounds.height();
            relY++) {
            u8 tilePGId = getPowerGroupID(sector, relX, relY);

            if (tilePGId == 0) {
                currentPGId = 0;
            } else if (tilePGId == currentPGId) {
                // Extend it
                currentBoundary->set_height(currentBoundary->height() + 1);
            } else {
                currentPGId = tilePGId;

                // Start a new boundary
                currentBoundary = sector->powerGroups.get(currentPGId - 1)->sectorBoundaries.appendBlank();
                currentBoundary->x = sector->bounds.x - 1;
                currentBoundary->y = sector->bounds.y + relY;
                currentBoundary->set_width(1);
                currentBoundary->set_height(1);
            }
        }
    }

    // - Step 4.2: Right edge
    if (sector->bounds.x + sector->bounds.width() < city->bounds.width()) {
        u8 currentPGId = 0;
        Rect2I* currentBoundary = nullptr;

        s32 relX = sector->bounds.width() - 1;
        for (s32 relY = 0;
            relY < sector->bounds.height();
            relY++) {
            u8 tilePGId = getPowerGroupID(sector, relX, relY);

            if (tilePGId == 0) {
                currentPGId = 0;
            } else if (tilePGId == currentPGId) {
                // Extend it
                currentBoundary->set_height(currentBoundary->height() + 1);
            } else {
                currentPGId = tilePGId;

                // Start a new boundary
                currentBoundary = sector->powerGroups.get(currentPGId - 1)->sectorBoundaries.appendBlank();
                currentBoundary->x = sector->bounds.x + sector->bounds.width();
                currentBoundary->y = sector->bounds.y + relY;
                currentBoundary->set_width(1);
                currentBoundary->set_height(1);
            }
        }
    }

    // - Step 4.3: Top edge
    if (sector->bounds.y > 0) {
        u8 currentPGId = 0;
        Rect2I* currentBoundary = nullptr;

        s32 relY = 0;
        for (s32 relX = 0;
            relX < sector->bounds.width();
            relX++) {
            u8 tilePGId = getPowerGroupID(sector, relX, relY);

            if (tilePGId == 0) {
                currentPGId = 0;
            } else if (tilePGId == currentPGId) {
                // Extend it
                currentBoundary->set_width(currentBoundary->width() + 1);
            } else {
                currentPGId = tilePGId;

                // Start a new boundary
                currentBoundary = sector->powerGroups.get(currentPGId - 1)->sectorBoundaries.appendBlank();
                currentBoundary->x = sector->bounds.x + relX;
                currentBoundary->y = sector->bounds.y - 1;
                currentBoundary->set_width(1);
                currentBoundary->set_height(1);
            }
        }
    }

    // - Step 4.4: Bottom edge
    if (sector->bounds.y + sector->bounds.height() < city->bounds.height()) {
        u8 currentPGId = 0;
        Rect2I* currentBoundary = nullptr;

        s32 relY = sector->bounds.height() - 1;
        for (s32 relX = 0;
            relX < sector->bounds.width();
            relX++) {
            u8 tilePGId = getPowerGroupID(sector, relX, relY);

            if (tilePGId == 0) {
                currentPGId = 0;
            } else if (tilePGId == currentPGId) {
                // Extend it
                currentBoundary->set_width(currentBoundary->width() + 1);
            } else {
                currentPGId = tilePGId;

                // Start a new boundary
                currentBoundary = sector->powerGroups.get(currentPGId - 1)->sectorBoundaries.appendBlank();
                currentBoundary->x = sector->bounds.x + relX;
                currentBoundary->y = sector->bounds.y + sector->bounds.height();
                currentBoundary->set_width(1);
                currentBoundary->set_height(1);
            }
        }
    }

    //
    // Step 5? Could now store a list of direct pointers or indices to connected power groups in other sectors.
    // We'd need to then clean those references up in step 0, but it could make things faster - no need to
    // query for the (0 to many) power groups covered by the boundaries whenever we need to walk the graph.
    // However, we probably don't need to walk it very often! Right now, I can only think of the "assign each
    // power group to a global network" procedure as needing to do this. (I guess when calculating which sectors
    // get power if the power prod < consumption, that also requires a walk? But maybe not. I really don't know
    // how I want that to work.)
    //
    // So, yeah! Could be useful or could not, but THIS is the place to do that work. (Or maybe directly in step 4?)
    // Though, we'd need to make sure we add the references on both sides, because otherwise things could go out
    // of sync, because we'd be building those links before some sectors have been calculated. UGH. Maybe it IS
    // better to just do that walk after we're all done!
    //
    // Sam, 12/06/2019
    //
}

void floodFillCityPowerNetwork(PowerLayer* layer, PowerGroup* powerGroup, PowerNetwork* network)
{
    DEBUG_FUNCTION();

    powerGroup->networkID = network->id;
    network->groups.append(powerGroup);

    for (auto it = powerGroup->sectorBoundaries.iterate();
        it.hasNext();
        it.next()) {
        Rect2I bounds = it.getValue();
        PowerSector* sector = getSectorAtTilePos(&layer->sectors, bounds.x, bounds.y);
        bounds = sector->bounds.intersected_relative(bounds);

        s32 lastPowerGroupIndex = -1;

        // TODO: @Speed We could probably just do 1 loop because the bounds rect is only 1-wide in one dimension!
        for (s32 relY = bounds.y; relY < bounds.y + bounds.height(); relY++) {
            for (s32 relX = bounds.x; relX < bounds.x + bounds.width(); relX++) {
                s32 powerGroupIndex = getPowerGroupID(sector, relX, relY);
                if (powerGroupIndex != 0 && powerGroupIndex != lastPowerGroupIndex) {
                    lastPowerGroupIndex = powerGroupIndex;
                    PowerGroup* group = sector->powerGroups.get(powerGroupIndex - 1);
                    if (group->networkID != network->id) {
                        floodFillCityPowerNetwork(layer, group, network);
                    }
                }
            }
        }
    }
}

void recalculatePowerConnectivity(PowerLayer* layer)
{
    DEBUG_FUNCTION();

    // Clean up networks
    for (auto networkIt = layer->networks.iterate();
        networkIt.hasNext();
        networkIt.next()) {
        PowerNetwork* powerNetwork = networkIt.get();

        for (auto groupIt = powerNetwork->groups.iterate();
            groupIt.hasNext();
            groupIt.next()) {
            PowerGroup* group = groupIt.getValue();
            group->networkID = 0;
        }

        freePowerNetwork(powerNetwork);
    }
    layer->networks.clear();

    // NB: All power groups are on networkID=0 right now, because they all got reconstructed in the above loop.
    // At some point we'll have to manually set that to 0, if we want to recalculate the global networks without
    // recalculating every individual sector.

    // Flood-fill networks of PowerGroups by walking the boundaries
    for (s32 sectorIndex = 0;
        sectorIndex < getSectorCount(&layer->sectors);
        sectorIndex++) {
        PowerSector* sector = &layer->sectors.sectors[sectorIndex];

        for (auto it = sector->powerGroups.iterate();
            it.hasNext();
            it.next()) {
            PowerGroup* powerGroup = it.get();
            if (powerGroup->networkID == 0) {
                PowerNetwork* network = newPowerNetwork(layer);
                floodFillCityPowerNetwork(layer, powerGroup, network);
            }
        }
    }
}

void updatePowerLayer(City* city, PowerLayer* layer)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    if (isDirty(&layer->dirtyRects)) {
        Set<PowerSector*> touchedSectors;
        initSet<PowerSector*>(&touchedSectors, &temp_arena(), [](PowerSector** a, PowerSector** b) { return *a == *b; });

        for (auto it = layer->dirtyRects.rects.iterate();
            it.hasNext();
            it.next()) {
            Rect2I dirtyRect = it.getValue();

            // Clear the "distance to power" for the surrounding area to 0 or 255
            for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.height(); y++) {
                for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.width(); x++) {
                    Building* building = getBuildingAt(city, x, y);
                    BuildingDef* def = nullptr;
                    if (building != nullptr) {
                        def = getBuildingDef(building->typeID);
                    }

                    if (def != nullptr && def->flags.has(BuildingFlags::CarriesPower)) {
                        layer->tilePowerDistance.set(x, y, 0);
                    } else if (ZONE_DEFS[getZoneAt(city, x, y)].carriesPower) {
                        layer->tilePowerDistance.set(x, y, 0);
                    } else {
                        layer->tilePowerDistance.set(x, y, 255);
                    }
                }
            }

            // Add the sectors to the list of touched sectors
            Rect2I sectorsRect = getSectorsCovered(&layer->sectors, dirtyRect);
            for (s32 sY = sectorsRect.y; sY < sectorsRect.y + sectorsRect.height(); sY++) {
                for (s32 sX = sectorsRect.x; sX < sectorsRect.x + sectorsRect.width(); sX++) {
                    touchedSectors.add(getSector(&layer->sectors, sX, sY));
                }
            }
        }

        // Recalculate distance
        updateDistances(&layer->tilePowerDistance, &layer->dirtyRects, layer->powerMaxDistance);

        // Rebuild the sectors that were modified
        for (auto it = touchedSectors.iterate(); it.hasNext(); it.next()) {
            PowerSector* sector = it.getValue();

            recalculateSectorPowerGroups(city, sector);
        }

        recalculatePowerConnectivity(layer);
        clearDirtyRects(&layer->dirtyRects);
    }

    layer->cachedCombinedProduction = 0;
    layer->cachedCombinedConsumption = 0;

    // Update each PowerGroup's power
    for (s32 sectorIndex = 0;
        sectorIndex < getSectorCount(&layer->sectors);
        sectorIndex++) {
        PowerSector* sector = &layer->sectors.sectors[sectorIndex];
        updateSectorPowerValues(city, sector);
    }

    // Sum each PowerGroup's power into its Network
    for (auto networkIt = layer->networks.iterate();
        networkIt.hasNext();
        networkIt.next()) {
        PowerNetwork* network = networkIt.get();
        network->cachedProduction = 0;
        network->cachedConsumption = 0;

        for (auto groupIt = network->groups.iterate();
            groupIt.hasNext();
            groupIt.next()) {
            PowerGroup* powerGroup = groupIt.getValue();
            network->cachedProduction += powerGroup->production;
            network->cachedConsumption += powerGroup->consumption;
        }

        // City-wide power totals
        layer->cachedCombinedProduction += network->cachedProduction;
        layer->cachedCombinedConsumption += network->cachedConsumption;
    }

    // Supply power to buildings
    for (auto networkIt = layer->networks.iterate();
        networkIt.hasNext();
        networkIt.next()) {
        PowerNetwork* network = networkIt.get();

        // Figure out which mode this network is in.
        enum class NetworkMode : u8 {
            Blackout,    // No production
            Brownout,    // Not enough production
            FullCoverage // Enough
        } networkMode;
        if (network->cachedConsumption == 0) {
            // No iteration of buildings is necessary!
            continue;
        }
        if (network->cachedProduction == 0) {
            networkMode = NetworkMode::Blackout;
        } else if (network->cachedProduction < network->cachedConsumption) {
            networkMode = NetworkMode::Brownout;
        } else {
            networkMode = NetworkMode::FullCoverage;
        }

        // Now, iterate the buildings and give them power based on that mode.
        s32 powerRemaining = network->cachedProduction;
        for (auto groupIt = network->groups.iterate();
            groupIt.hasNext();
            groupIt.next()) {
            PowerGroup* powerGroup = groupIt.getValue();

            for (auto buildingRefIt = powerGroup->buildings.iterate();
                buildingRefIt.hasNext();
                buildingRefIt.next()) {
                BuildingRef buildingRef = buildingRefIt.getValue();
                Building* building = getBuilding(city, buildingRef);

                if (building != nullptr) {
                    switch (networkMode) {
                    case NetworkMode::Blackout: {
                        building->allocatedPower = 0;
                    } break;

                    case NetworkMode::Brownout: {
                        // Supply power to buildings if we can, and mark the rest as unpowered.
                        // TODO: Implement some kind of "rolling brownout" system where buildings get powered
                        // and unpowered over time to even things out, instead of it always being first-come-first-served.
                        s32 requiredPower = getRequiredPower(building);
                        if (powerRemaining >= requiredPower) {
                            building->allocatedPower = requiredPower;
                            powerRemaining -= requiredPower;
                        } else {
                            building->allocatedPower = 0;
                        }
                    } break;

                    case NetworkMode::FullCoverage: {
                        building->allocatedPower = getRequiredPower(building);
                    } break;
                    }
                }
            }
        }
    }
}

void notifyNewBuilding(PowerLayer* layer, BuildingDef* def, Building* building)
{
    if (def->power > 0) {
        layer->powerBuildings.append(getReferenceTo(building));
    }
}

void notifyBuildingDemolished(PowerLayer* layer, BuildingDef* def, Building* building)
{
    if (def->power > 0) {
        bool success = layer->powerBuildings.findAndRemove(getReferenceTo(building));
        ASSERT(success);
    }
}

void debugInspectPower(UI::Panel* panel, City* city, s32 x, s32 y)
{
    panel->addLabel("*** POWER INFO ***"_s);

    // Power group
    PowerNetwork* powerNetwork = getPowerNetworkAt(city, x, y);
    if (powerNetwork != nullptr) {
        panel->addLabel(myprintf("Power Network {0}:\n- Production: {1}\n- Consumption: {2}\n- Contained groups: {3}"_s, { formatInt(powerNetwork->id), formatInt(powerNetwork->cachedProduction), formatInt(powerNetwork->cachedConsumption), formatInt(powerNetwork->groups.count) }));
    }

    panel->addLabel(myprintf("Distance to power: {0}"_s, { formatInt(getDistanceToPower(city, x, y)) }));
}
