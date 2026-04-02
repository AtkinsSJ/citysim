/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Crime.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Menus/SaveFile.h>
#include <Sim/City.h>
#include <Sim/Effect.h>
#include <Sim/LandValue.h>

CrimeLayer::CrimeLayer(City& city, MemoryArena& arena)
    : m_dirty_rects(arena, city.bounds)
    , m_sectors(SectorGrid<BasicSector> { &arena, city.bounds.size(), 16, 8 })
    , m_tile_police_coverage(arena.allocate_array_2d<u8>(city.bounds.size()))
    , m_police_buildings(city.buildingRefsChunkPool)
    , m_total_jail_capacity(0)
    , m_occupied_jail_capacity(0)
    , m_funding_level(1.0f)
{
    m_tile_police_coverage.fill(0);
}

void CrimeLayer::update(City& city)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    if (m_dirty_rects.is_dirty()) {
        DEBUG_BLOCK_T("updateCrimeLayer: dirty rects", DebugCodeDataTag::Simulation);
        m_dirty_rects.clear();
    }

    // Recalculate jail capacity
    // NB: This only makes sense if we assume that building jail capacities can change while
    // the game is running. It'll only happen incredibly rarely during normal play, but during
    // development we have this whole hot-loaded building defs system, so it's better to be safe.
    m_total_jail_capacity = 0;
    for (auto it = m_police_buildings.iterate(); it.hasNext(); it.next()) {
        Building* building = city.get_building(it.getValue());
        if (building != nullptr) {
            if (auto& def = building->get_def(); def.jailCapacity > 0)
                m_total_jail_capacity += def.jailCapacity;
        }
    }

    {
        DEBUG_BLOCK_T("updateCrimeLayer: sector updates", DebugCodeDataTag::Simulation);

        for (s32 i = 0; i < m_sectors.sectors_to_update_per_tick(); i++) {
            auto [_, sector] = m_sectors.get_next_sector();

            DEBUG_BLOCK_T("updateCrimeLayer: building police coverage", DebugCodeDataTag::Simulation);
            m_tile_police_coverage.fill_region(sector.bounds, 0);
            for (auto it = m_police_buildings.iterate(); it.hasNext(); it.next()) {
                Building* building = city.get_building(it.getValue());
                if (building != nullptr) {
                    if (auto& def = building->get_def(); def.policeEffect.has_effect()) {
                        // Budget
                        float effectiveness = m_funding_level;

                        if (!building->has_power()) {
                            effectiveness *= 0.4f; // @Balance

                            // TODO: Consider water access too
                        }

                        def.policeEffect.apply(m_tile_police_coverage, sector.bounds, building->footprint.centre(), EffectType::Add, effectiveness);
                    }
                }
            }
        }
    }
}

void CrimeLayer::mark_dirty(Rect2I bounds)
{
    // FIXME: Why are we using the land-value radius?
    m_dirty_rects.mark_dirty(bounds.expanded(maxLandValueEffectDistance));
}

void CrimeLayer::notify_new_building(BuildingDef const& def, Building& building)
{
    if (def.policeEffect.has_effect() || (def.jailCapacity > 0)) {
        m_police_buildings.append(building.get_reference());
    }
}

void CrimeLayer::notify_building_demolished(BuildingDef const& def, Building& building)
{
    if (def.policeEffect.has_effect() || (def.jailCapacity > 0)) {
        bool success = m_police_buildings.findAndRemove(building.get_reference());
        ASSERT(success);
    }
}

float CrimeLayer::get_police_coverage_percent_at(s32 x, s32 y) const
{
    return m_tile_police_coverage.get(x, y) / 255.0f;
}

void CrimeLayer::save(BinaryFileWriter& writer) const
{
    writer.startSection<SAVSection_Crime>(SAV_CRIME_ID, SAV_CRIME_VERSION);
    SAVSection_Crime crimeSection = {};

    crimeSection.totalJailCapacity = m_total_jail_capacity;
    crimeSection.occupiedJailCapacity = m_occupied_jail_capacity;

    writer.endSection<SAVSection_Crime>(&crimeSection);
}

bool CrimeLayer::load(BinaryFileReader& reader, City&)
{
    bool succeeded = false;
    while (reader.startSection(SAV_CRIME_ID, SAV_CRIME_VERSION)) {
        // Load Crime
        SAVSection_Crime* section = reader.readStruct<SAVSection_Crime>(0);
        if (!section)
            break;

        m_total_jail_capacity = section->totalJailCapacity;
        m_occupied_jail_capacity = section->occupiedJailCapacity;

        succeeded = true;
        break;
    }

    return succeeded;
}
