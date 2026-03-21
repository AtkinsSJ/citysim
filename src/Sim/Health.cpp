/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Health.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Menus/SaveFile.h>
#include <Sim/City.h>
#include <Sim/Effect.h>

HealthLayer::HealthLayer(City& city, MemoryArena& arena)
    : m_dirty_rects(arena, city.bounds)
{
    m_sectors = SectorGrid<BasicSector> { &arena, city.bounds.size(), 16, 8 };

    m_tile_health_coverage = arena.allocate_array_2d<u8>(city.bounds.size());
    m_tile_health_coverage.fill(0);

    initChunkedArray(&m_health_buildings, &city.buildingRefsChunkPool);

    m_funding_level = 1.0f;
}

void HealthLayer::update(City& city)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    if (m_dirty_rects.is_dirty()) {
        DEBUG_BLOCK_T("updateHealthLayer: dirty rects", DebugCodeDataTag::Simulation);

        // TODO: do we actually need dirty rects? I can't think of anything, unless we move the "register building" stuff to that.

        m_dirty_rects.clear();
    }

    {
        DEBUG_BLOCK_T("updateHealthLayer: sector updates", DebugCodeDataTag::Simulation);

        for (s32 i = 0; i < m_sectors.sectors_to_update_per_tick(); i++) {
            auto [_, sector] = m_sectors.get_next_sector();

            DEBUG_BLOCK_T("updateHealthLayer: building health coverage", DebugCodeDataTag::Simulation);
            m_tile_health_coverage.fill_region(sector.bounds, 0);
            for (auto it = m_health_buildings.iterate(); it.hasNext(); it.next()) {
                Building* building = city.get_building(it.getValue());
                if (building != nullptr) {
                    BuildingDef* def = getBuildingDef(building);

                    // Budget
                    float effectiveness = m_funding_level;

                    // Power
                    if (!buildingHasPower(building)) {
                        effectiveness *= 0.4f; // @Balance
                    }

                    // TODO: Water

                    // TODO: Overcrowding

                    def->healthEffect.apply(m_tile_health_coverage, sector.bounds, building->footprint.centre(), EffectType::Max, effectiveness);
                }
            }
        }
    }
}

void HealthLayer::mark_dirty(Rect2I bounds)
{
    // FIXME: Why are we using the land-value radius?
    m_dirty_rects.mark_dirty(bounds.expanded(maxLandValueEffectDistance));
}

void HealthLayer::notify_new_building(BuildingDef const& def, Building& building)
{
    if (def.healthEffect.has_effect()) {
        m_health_buildings.append(building.get_reference());
    }
}

void HealthLayer::notify_building_demolished(BuildingDef const& def, Building& building)
{
    if (def.healthEffect.has_effect()) {
        bool success = m_health_buildings.findAndRemove(building.get_reference());
        ASSERT(success);
    }
}

float HealthLayer::get_health_coverage_percent_at(s32 x, s32 y) const
{
    return m_tile_health_coverage.get(x, y) * 0.01f;
}

void HealthLayer::save(BinaryFileWriter& writer) const
{
    writer.startSection<SAVSection_Health>(SAV_HEALTH_ID, SAV_HEALTH_VERSION);
    SAVSection_Health healthSection = {};

    writer.endSection<SAVSection_Health>(&healthSection);
}

bool HealthLayer::load(BinaryFileReader& reader)
{
    bool succeeded = false;
    while (reader.startSection(SAV_HEALTH_ID, SAV_HEALTH_VERSION)) {
        SAVSection_Health* section = reader.readStruct<SAVSection_Health>(0);
        if (!section)
            break;

        succeeded = true;
        break;
    }

    return succeeded;
}
