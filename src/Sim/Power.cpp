/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Power.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Menus/SaveFile.h>
#include <Sim/City.h>
#include <UI/Panel.h>
#include <Util/Set.h>

PowerLayer::PowerLayer(City& city, MemoryArena& arena)
    : m_bounds(city.bounds)
    , m_dirty_rects(arena, m_bounds)
{
    initChunkedArray(&m_networks, &arena, 64);
    initChunkPool(&m_power_groups_chunk_pool, &arena, 4);
    initChunkPool(&m_power_group_pointers_chunk_pool, &arena, 32);

    m_tile_power_distance = arena.allocate_array_2d<u8>(m_bounds.size());
    m_tile_power_distance.fill(255);

    m_sectors = SectorGrid<PowerSector> { &arena, m_bounds.size(), 16, 0 };
    for (s32 sectorIndex = 0; sectorIndex < m_sectors.sector_count(); sectorIndex++) {
        PowerSector* sector = m_sectors.get_by_index(sectorIndex);

        sector->tilePowerGroup = arena.allocate_array_2d<u8>(sector->bounds.size());

        initChunkedArray(&sector->powerGroups, &m_power_groups_chunk_pool);
    }

    initChunkedArray(&m_power_buildings, &city.buildingRefsChunkPool);
}

PowerNetwork& PowerLayer::new_power_network()
{
    PowerNetwork* network = m_networks.appendBlank();
    network->id = m_networks.count;
    initChunkedArray(&network->groups, &m_power_group_pointers_chunk_pool);

    return *network;
}

void PowerLayer::free_power_network(PowerNetwork& network)
{
    network.id = 0;
    network.groups.clear();
}

u8 PowerSector::get_power_group_id(s32 relX, s32 relY) const
{
    return tilePowerGroup.get(relX, relY);
}

void PowerSector::set_power_group_id(s32 relX, s32 relY, u8 value)
{
    tilePowerGroup.set(relX, relY, value);
}

PowerGroup* PowerSector::get_power_group_at(s32 relX, s32 relY)
{
    PowerGroup* result = nullptr;

    s32 powerGroupID = get_power_group_id(relX, relY);
    if (powerGroupID != 0 && powerGroupID != POWER_GROUP_UNKNOWN) {
        result = &powerGroups.get(powerGroupID - 1);
    }

    return result;
}

u8 PowerLayer::get_distance_to_power(s32 x, s32 y) const
{
    return m_tile_power_distance.get(x, y);
}

u8 PowerLayer::calculate_power_overlay_for_tile(s32 x, s32 y) const
{
    u8 paletteIndexPowered = 0;
    u8 paletteIndexBrownout = 1;
    u8 paletteIndexBlackout = 2;
    u8 paletteIndexNone = 255;

    u8 result = paletteIndexNone;

    auto* network = get_power_network_at(x, y);
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

void PowerSector::update_power_values(City& city)
{
    DEBUG_FUNCTION();

    // Reset each to 0
    for (auto it = powerGroups.iterate();
        it.hasNext();
        it.next()) {
        auto& powerGroup = it.get();
        powerGroup.production = 0;
        powerGroup.consumption = 0;
    }

    // Count power from buildings
    city.for_each_building_overlapping_area(bounds, BuildingQueryFlag::RequireOriginInArea, [&](auto& building) {
        auto& def = building.get_def();
        if (def.power != 0) {
            u8 powerGroupIndex = get_power_group_id(building.footprint.x() - bounds.x(), building.footprint.y() - bounds.y());
            auto& power_group = powerGroups.get(powerGroupIndex - 1);
            if (def.power > 0) {
                power_group.production += def.power;
            } else {
                power_group.consumption -= def.power;
            }
        }
    });
}

bool PowerLayer::does_tile_have_power_network(s32 x, s32 y) const
{
    bool result = false;

    if (m_bounds.contains(x, y)) {
        auto* sector = m_sectors.get_sector_at_tile_pos(x, y);

        s32 relX = x - sector->bounds.x();
        s32 relY = y - sector->bounds.y();

        u8 powerGroupIndex = sector->get_power_group_id(relX, relY);
        result = (powerGroupIndex != 0);
    }

    return result;
}

PowerNetwork const* PowerLayer::get_power_network_at(s32 x, s32 y) const
{
    PowerNetwork const* result = nullptr;

    if (m_bounds.contains(x, y)) {
        auto* sector = m_sectors.get_sector_at_tile_pos(x, y);

        s32 relX = x - sector->bounds.x();
        s32 relY = y - sector->bounds.y();

        u8 powerGroupIndex = sector->get_power_group_id(relX, relY);
        if (powerGroupIndex != 0) {
            auto& group = sector->powerGroups.get(powerGroupIndex - 1);
            result = &m_networks.get(group.networkID - 1);
        }
    }

    return result;
}

void PowerSector::flood_fill_power_group(s32 x, s32 y, u8 fillValue)
{
    DEBUG_FUNCTION();

    // Theoretically, the only place we non-recursively call this is in recalculate_sector_power_groups(),
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

    set_power_group_id(x, y, fillValue);

    if ((x > 0) && (get_power_group_id(x - 1, y) == POWER_GROUP_UNKNOWN)) {
        flood_fill_power_group(x - 1, y, fillValue);
    }

    if ((x < bounds.width() - 1) && (get_power_group_id(x + 1, y) == POWER_GROUP_UNKNOWN)) {
        flood_fill_power_group(x + 1, y, fillValue);
    }

    if ((y > 0) && (get_power_group_id(x, y - 1) == POWER_GROUP_UNKNOWN)) {
        flood_fill_power_group(x, y - 1, fillValue);
    }

    if ((y < bounds.height() - 1) && (get_power_group_id(x, y + 1) == POWER_GROUP_UNKNOWN)) {
        flood_fill_power_group(x, y + 1, fillValue);
    }
}

void PowerSector::set_rect_power_group_unknown(Rect2I area)
{
    DEBUG_FUNCTION();

    Rect2I relArea = bounds.intersected_relative(area);

    for (s32 relY = relArea.y();
        relY < relArea.y() + relArea.height();
        relY++) {
        for (s32 relX = relArea.x();
            relX < relArea.x() + relArea.width();
            relX++) {
            set_power_group_id(relX, relY, POWER_GROUP_UNKNOWN);
        }
    }
}

void PowerLayer::mark_dirty(Rect2I bounds)
{
    m_dirty_rects.mark_dirty(bounds.expanded(m_power_max_distance));
}

void PowerLayer::recalculate_sector_power_groups(City& city, PowerSector& sector)
{
    DEBUG_FUNCTION();

    // TODO: Clear any references to the PowerGroups that the City itself might have!
    // (I don't know how that's going to be structured yet.)
    // Meaning, if a city-wide power network knows that PowerGroup 3 in this sector is part of it,
    // we need to tell it that PowerGroup 3 is being destroyed!

    // Step 0: Remove the old PowerGroups.
    for (auto it = sector.powerGroups.iterate(); it.hasNext(); it.next()) {
        auto& powerGroup = it.get();
        powerGroup.sectorBoundaries.clear();
    }
    sector.powerGroups.clear();
    sector.tilePowerGroup.fill(0);

    // Step 1: Set all power-carrying tiles to POWER_GROUP_UNKNOWN (everything was set to 0 in the above memset())
    for (s32 relY = 0;
        relY < sector.bounds.height();
        relY++) {
        for (s32 relX = 0;
            relX < sector.bounds.width();
            relX++) {
            u8 distanceToPower = m_tile_power_distance.get(relX + sector.bounds.x(), relY + sector.bounds.y());

            if (distanceToPower <= 1) {
                sector.set_power_group_id(relX, relY, POWER_GROUP_UNKNOWN);
            } else {
                sector.set_power_group_id(relX, relY, 0);
            }
        }
    }

    // Step 2: Flood fill each -1 tile as a local PowerGroup
    for (s32 relY = 0;
        relY < sector.bounds.height();
        relY++) {
        for (s32 relX = 0;
            relX < sector.bounds.width();
            relX++) {
            // Skip tiles that have already been added to a PowerGroup
            if (sector.get_power_group_id(relX, relY) != POWER_GROUP_UNKNOWN)
                continue;

            PowerGroup* newGroup = sector.powerGroups.appendBlank();

            initChunkedArray(&newGroup->sectorBoundaries, &city.sectorBoundariesChunkPool);
            initChunkedArray(&newGroup->buildings, &city.buildingRefsChunkPool);

            u8 powerGroupID = (u8)sector.powerGroups.count;
            newGroup->production = 0;
            newGroup->consumption = 0;
            sector.flood_fill_power_group(relX, relY, powerGroupID);
        }
    }

    // At this point, if there are no power groups we can just stop.
    if (sector.powerGroups.is_empty())
        return;

    // Store references to the buildings in each group, for faster updating later
    city.for_each_building_overlapping_area(sector.bounds, {}, [&](auto& building) {
        auto& def = building.get_def();
        if (def.power == 0)
            return; // We only care about powered buildings!

        if (sector.bounds.contains(building.footprint.position())) {
            PowerGroup* group = sector.get_power_group_at(building.footprint.x() - sector.bounds.x(), building.footprint.y() - sector.bounds.y());

            ASSERT(group != nullptr);
            group->buildings.append(building.get_reference());
        }
    });

    // Step 3: Calculate power production/consumption for OWNED buildings, and add to their PowerGroups
    sector.update_power_values(city);

    // Step 4: Find and store the PowerGroup boundaries along the sector's edges, on the OUTSIDE
    // @Copypasta The code for all this is really repetitive, but I'm not sure how to factor it together nicely.

    // - Step 4.1: Left edge
    if (sector.bounds.x() > 0) {
        u8 currentPGId = 0;
        Rect2I* currentBoundary = nullptr;

        s32 relX = 0;
        for (s32 relY = 0;
            relY < sector.bounds.height();
            relY++) {
            u8 tilePGId = sector.get_power_group_id(relX, relY);

            if (tilePGId == 0) {
                currentPGId = 0;
            } else if (tilePGId == currentPGId) {
                // Extend it
                currentBoundary->set_height(currentBoundary->height() + 1);
            } else {
                currentPGId = tilePGId;

                // Start a new boundary
                currentBoundary = sector.powerGroups.get(currentPGId - 1).sectorBoundaries.appendBlank();
                currentBoundary->set_x(sector.bounds.x() - 1);
                currentBoundary->set_y(sector.bounds.y() + relY);
                currentBoundary->set_width(1);
                currentBoundary->set_height(1);
            }
        }
    }

    // - Step 4.2: Right edge
    if (sector.bounds.x() + sector.bounds.width() < m_bounds.width()) {
        u8 currentPGId = 0;
        Rect2I* currentBoundary = nullptr;

        s32 relX = sector.bounds.width() - 1;
        for (s32 relY = 0;
            relY < sector.bounds.height();
            relY++) {
            u8 tilePGId = sector.get_power_group_id(relX, relY);

            if (tilePGId == 0) {
                currentPGId = 0;
            } else if (tilePGId == currentPGId) {
                // Extend it
                currentBoundary->set_height(currentBoundary->height() + 1);
            } else {
                currentPGId = tilePGId;

                // Start a new boundary
                currentBoundary = sector.powerGroups.get(currentPGId - 1).sectorBoundaries.appendBlank();
                currentBoundary->set_x(sector.bounds.x() + sector.bounds.width());
                currentBoundary->set_y(sector.bounds.y() + relY);
                currentBoundary->set_width(1);
                currentBoundary->set_height(1);
            }
        }
    }

    // - Step 4.3: Top edge
    if (sector.bounds.y() > 0) {
        u8 currentPGId = 0;
        Rect2I* currentBoundary = nullptr;

        s32 relY = 0;
        for (s32 relX = 0;
            relX < sector.bounds.width();
            relX++) {
            u8 tilePGId = sector.get_power_group_id(relX, relY);

            if (tilePGId == 0) {
                currentPGId = 0;
            } else if (tilePGId == currentPGId) {
                // Extend it
                currentBoundary->set_width(currentBoundary->width() + 1);
            } else {
                currentPGId = tilePGId;

                // Start a new boundary
                currentBoundary = sector.powerGroups.get(currentPGId - 1).sectorBoundaries.appendBlank();
                currentBoundary->set_x(sector.bounds.x() + relX);
                currentBoundary->set_y(sector.bounds.y() - 1);
                currentBoundary->set_width(1);
                currentBoundary->set_height(1);
            }
        }
    }

    // - Step 4.4: Bottom edge
    if (sector.bounds.y() + sector.bounds.height() < m_bounds.height()) {
        u8 currentPGId = 0;
        Rect2I* currentBoundary = nullptr;

        s32 relY = sector.bounds.height() - 1;
        for (s32 relX = 0;
            relX < sector.bounds.width();
            relX++) {
            u8 tilePGId = sector.get_power_group_id(relX, relY);

            if (tilePGId == 0) {
                currentPGId = 0;
            } else if (tilePGId == currentPGId) {
                // Extend it
                currentBoundary->set_width(currentBoundary->width() + 1);
            } else {
                currentPGId = tilePGId;

                // Start a new boundary
                currentBoundary = sector.powerGroups.get(currentPGId - 1).sectorBoundaries.appendBlank();
                currentBoundary->set_x(sector.bounds.x() + relX);
                currentBoundary->set_y(sector.bounds.y() + sector.bounds.height());
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

void PowerLayer::flood_fill_city_power_network(PowerGroup& powerGroup, PowerNetwork& network)
{
    DEBUG_FUNCTION();

    powerGroup.networkID = network.id;
    network.groups.append(&powerGroup);

    for (auto it = powerGroup.sectorBoundaries.iterate();
        it.hasNext();
        it.next()) {
        Rect2I bounds = it.getValue();
        PowerSector* sector = m_sectors.get_sector_at_tile_pos(bounds.x(), bounds.y());
        bounds = sector->bounds.intersected_relative(bounds);

        s32 lastPowerGroupIndex = -1;

        // TODO: @Speed We could probably just do 1 loop because the bounds rect is only 1-wide in one dimension!
        for (s32 relY = bounds.y(); relY < bounds.y() + bounds.height(); relY++) {
            for (s32 relX = bounds.x(); relX < bounds.x() + bounds.width(); relX++) {
                s32 powerGroupIndex = sector->get_power_group_id(relX, relY);
                if (powerGroupIndex != 0 && powerGroupIndex != lastPowerGroupIndex) {
                    lastPowerGroupIndex = powerGroupIndex;
                    auto& group = sector->powerGroups.get(powerGroupIndex - 1);
                    if (group.networkID != network.id) {
                        flood_fill_city_power_network(group, network);
                    }
                }
            }
        }
    }
}

void PowerLayer::recalculate_power_connectivity()
{
    DEBUG_FUNCTION();

    // Clean up networks
    for (auto networkIt = m_networks.iterate();
        networkIt.hasNext();
        networkIt.next()) {
        auto& powerNetwork = networkIt.get();

        for (auto groupIt = powerNetwork.groups.iterate();
            groupIt.hasNext();
            groupIt.next()) {
            PowerGroup* group = groupIt.getValue();
            group->networkID = 0;
        }

        free_power_network(powerNetwork);
    }
    m_networks.clear();

    // NB: All power groups are on networkID=0 right now, because they all got reconstructed in the above loop.
    // At some point we'll have to manually set that to 0, if we want to recalculate the global networks without
    // recalculating every individual sector.

    // Flood-fill networks of PowerGroups by walking the boundaries
    for (s32 sectorIndex = 0;
        sectorIndex < m_sectors.sector_count();
        sectorIndex++) {
        PowerSector* sector = m_sectors.get_by_index(sectorIndex);

        for (auto it = sector->powerGroups.iterate();
            it.hasNext();
            it.next()) {
            auto& powerGroup = it.get();
            if (powerGroup.networkID == 0) {
                auto& network = new_power_network();
                flood_fill_city_power_network(powerGroup, network);
            }
        }
    }
}

void PowerLayer::update(City& city)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    if (m_dirty_rects.is_dirty()) {
        Set<PowerSector*> touchedSectors;
        initSet<PowerSector*>(&touchedSectors, &temp_arena(), [](PowerSector** a, PowerSector** b) { return *a == *b; });

        for (auto it = m_dirty_rects.rects().iterate();
            it.hasNext();
            it.next()) {
            Rect2I dirtyRect = it.getValue();

            // Clear the "distance to power" for the surrounding area to 0 or 255
            for (s32 y = dirtyRect.y(); y < dirtyRect.y() + dirtyRect.height(); y++) {
                for (s32 x = dirtyRect.x(); x < dirtyRect.x() + dirtyRect.width(); x++) {
                    Building* building = city.get_building_at(x, y);
                    BuildingDef const* def = nullptr;
                    if (building != nullptr) {
                        def = &building->get_def();
                    }

                    if (def != nullptr && def->flags.has(BuildingFlags::CarriesPower)) {
                        m_tile_power_distance.set(x, y, 0);
                    } else if (ZONE_DEFS[city.zoneLayer.get_zone_at(x, y)].carriesPower) {
                        m_tile_power_distance.set(x, y, 0);
                    } else {
                        m_tile_power_distance.set(x, y, 255);
                    }
                }
            }

            // Add the sectors to the list of touched sectors
            Rect2I sectorsRect = m_sectors.get_sectors_covered(dirtyRect);
            for (s32 sY = sectorsRect.y(); sY < sectorsRect.y() + sectorsRect.height(); sY++) {
                for (s32 sX = sectorsRect.x(); sX < sectorsRect.x() + sectorsRect.width(); sX++) {
                    touchedSectors.add(m_sectors.get(sX, sY));
                }
            }
        }

        // Recalculate distance
        updateDistances(&m_tile_power_distance, &m_dirty_rects, m_power_max_distance);

        // Rebuild the sectors that were modified
        for (auto it = touchedSectors.iterate(); it.hasNext(); it.next()) {
            PowerSector* sector = it.getValue();

            recalculate_sector_power_groups(city, *sector);
        }

        recalculate_power_connectivity();
        m_dirty_rects.clear();
    }

    m_cached_combined_production = 0;
    m_cached_combined_consumption = 0;

    // Update each PowerGroup's power
    for (s32 sectorIndex = 0;
        sectorIndex < m_sectors.sector_count();
        sectorIndex++) {
        PowerSector* sector = m_sectors.get_by_index(sectorIndex);
        sector->update_power_values(city);
    }

    // Sum each PowerGroup's power into its Network
    for (auto networkIt = m_networks.iterate();
        networkIt.hasNext();
        networkIt.next()) {
        auto& network = networkIt.get();
        network.cachedProduction = 0;
        network.cachedConsumption = 0;

        for (auto groupIt = network.groups.iterate();
            groupIt.hasNext();
            groupIt.next()) {
            PowerGroup* powerGroup = groupIt.getValue();
            network.cachedProduction += powerGroup->production;
            network.cachedConsumption += powerGroup->consumption;
        }

        // City-wide power totals
        m_cached_combined_production += network.cachedProduction;
        m_cached_combined_consumption += network.cachedConsumption;
    }

    // Supply power to buildings
    for (auto networkIt = m_networks.iterate();
        networkIt.hasNext();
        networkIt.next()) {
        auto& network = networkIt.get();

        // Figure out which mode this network is in.
        enum class NetworkMode : u8 {
            Blackout,    // No production
            Brownout,    // Not enough production
            FullCoverage // Enough
        } networkMode;
        if (network.cachedConsumption == 0) {
            // No iteration of buildings is necessary!
            continue;
        }
        if (network.cachedProduction == 0) {
            networkMode = NetworkMode::Blackout;
        } else if (network.cachedProduction < network.cachedConsumption) {
            networkMode = NetworkMode::Brownout;
        } else {
            networkMode = NetworkMode::FullCoverage;
        }

        // Now, iterate the buildings and give them power based on that mode.
        s32 powerRemaining = network.cachedProduction;
        for (auto groupIt = network.groups.iterate();
            groupIt.hasNext();
            groupIt.next()) {
            PowerGroup* powerGroup = groupIt.getValue();

            for (auto buildingRefIt = powerGroup->buildings.iterate();
                buildingRefIt.hasNext();
                buildingRefIt.next()) {
                BuildingRef buildingRef = buildingRefIt.getValue();
                Building* building = city.get_building(buildingRef);

                if (building != nullptr) {
                    switch (networkMode) {
                    case NetworkMode::Blackout: {
                        building->allocatedPower = 0;
                    } break;

                    case NetworkMode::Brownout: {
                        // Supply power to buildings if we can, and mark the rest as unpowered.
                        // TODO: Implement some kind of "rolling brownout" system where buildings get powered
                        // and unpowered over time to even things out, instead of it always being first-come-first-served.
                        s32 requiredPower = building->required_power();
                        if (powerRemaining >= requiredPower) {
                            building->allocatedPower = requiredPower;
                            powerRemaining -= requiredPower;
                        } else {
                            building->allocatedPower = 0;
                        }
                    } break;

                    case NetworkMode::FullCoverage: {
                        building->allocatedPower = building->required_power();
                    } break;
                    }
                }
            }
        }
    }
}

void PowerLayer::notify_new_building(BuildingDef const& def, Building& building)
{
    if (def.power > 0) {
        m_power_buildings.append(building.get_reference());
    }
}

void PowerLayer::notify_building_demolished(BuildingDef const& def, Building& building)
{
    if (def.power > 0) {
        bool success = m_power_buildings.findAndRemove(building.get_reference());
        ASSERT(success);
    }
}

void PowerLayer::debug_inspect(UI::Panel& panel, V2I tile_position)
{
    panel.addLabel("*** POWER INFO ***"_s);

    // Power group
    auto* powerNetwork = get_power_network_at(tile_position.x, tile_position.y);
    if (powerNetwork != nullptr) {
        panel.addLabel(myprintf("Power Network {0}:\n- Production: {1}\n- Consumption: {2}\n- Contained groups: {3}"_s, { formatInt(powerNetwork->id), formatInt(powerNetwork->cachedProduction), formatInt(powerNetwork->cachedConsumption), formatInt(powerNetwork->groups.count) }));
    }

    panel.addLabel(myprintf("Distance to power: {0}"_s, { formatInt(get_distance_to_power(tile_position.x, tile_position.y)) }));
}
