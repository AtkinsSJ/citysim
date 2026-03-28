/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Pollution.h"
#include <IO/BinaryFile.h>
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Menus/SaveFile.h>
#include <Sim/Building.h>
#include <Sim/City.h>
#include <Sim/Effect.h>
#include <Sim/LandValue.h>

PollutionLayer::PollutionLayer(City& city, MemoryArena& arena)
    : m_dirty_rects(arena, city.bounds)
{
    m_tile_pollution = arena.allocate_array_2d<u8>(city.bounds.size());
    m_tile_pollution.fill(0);

    m_tile_building_contributions = arena.allocate_array_2d<s16>(city.bounds.size());
    m_tile_building_contributions.fill(0);
}

void PollutionLayer::update(City& city)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    if (m_dirty_rects.is_dirty()) {
        // @Copypasta from updateLandValueLayer()
        {
            DEBUG_BLOCK_T("updatePollutionLayer: building effects", DebugCodeDataTag::Simulation);

            // Recalculate the building contributions
            for (auto rectIt = m_dirty_rects.rects().iterate();
                rectIt.hasNext();
                rectIt.next()) {
                Rect2I dirtyRect = rectIt.getValue();

                m_tile_building_contributions.fill_region(dirtyRect, 0);

                city.for_each_building_overlapping_area(dirtyRect.expanded(maxLandValueEffectDistance), {}, [&](auto& building) {
                    auto& def = building.get_def();
                    def.pollutionEffect.apply(m_tile_building_contributions, dirtyRect, building.footprint.centre(), EffectType::Add);
                });

                // Now, clamp the tile values into the range we want!
                // The above process may have overflowed the -255 to 255 range we want the values to be,
                // but we don't clamp as we go along because then the end result might depend on the order that
                // buildings are processed.
                // eg, if we a total positive input of 1000 and a total negative of -900, the end result should be
                // +100, but if we did all the positives first and then all the negatives, we'd clamp at +255, and
                // then subtract the -900 and end up clamped to -255! (Or +255 if it happened the other way around.)
                // So, we instead give ourselves a lot of extra breathing room, and only clamp values at the very end.
                //
                // - Sam, 04/09/2019
                //
                for (s32 y = dirtyRect.y(); y < dirtyRect.y() + dirtyRect.height(); y++) {
                    for (s32 x = dirtyRect.x(); x < dirtyRect.x() + dirtyRect.width(); x++) {
                        s16 originalValue = m_tile_building_contributions.get(x, y);
                        s16 newValue = clamp<s16>(originalValue, -255, 255);
                        m_tile_building_contributions.set(x, y, newValue);
                    }
                }
            }
        }

        // Recalculate overall value
        {
            DEBUG_BLOCK_T("updatePollutionLayer: combine", DebugCodeDataTag::Simulation);

            for (auto rectIt = m_dirty_rects.rects().iterate();
                rectIt.hasNext();
                rectIt.next()) {
                Rect2I dirtyRect = rectIt.getValue();

                for (s32 y = dirtyRect.y(); y < dirtyRect.y() + dirtyRect.height(); y++) {
                    for (s32 x = dirtyRect.x(); x < dirtyRect.x() + dirtyRect.width(); x++) {
                        // This is really lazy right now.
                        // In future we might want to update things over time, but right now it's just
                        // a glorified copy from the tileBuildingContributions array.

                        s16 buildingContributions = m_tile_building_contributions.get(x, y);

                        u8 newValue = (u8)clamp<s16>(buildingContributions, 0, 255);

                        m_tile_pollution.set(x, y, newValue);
                    }
                }
            }
        }

        m_dirty_rects.clear();
    }
}

void PollutionLayer::mark_dirty(Rect2I bounds)
{
    m_dirty_rects.mark_dirty(bounds.expanded(maxPollutionEffectDistance));
}

float PollutionLayer::get_pollution_percent_at(s32 x, s32 y) const
{
    return m_tile_pollution.get(x, y) / 255.0f;
}

void PollutionLayer::save(BinaryFileWriter& writer) const
{
    writer.startSection<SAVSection_Pollution>(SAV_POLLUTION_ID, SAV_POLLUTION_VERSION);
    SAVSection_Pollution pollutionSection = {};

    // Tile pollution
    pollutionSection.tilePollution = writer.appendBlob(&m_tile_pollution, FileBlobCompressionScheme::RLE_S8);

    writer.endSection<SAVSection_Pollution>(&pollutionSection);
}

bool PollutionLayer::load(BinaryFileReader& reader, City&)
{
    bool succeeded = false;
    while (reader.startSection(SAV_POLLUTION_ID, SAV_POLLUTION_VERSION)) {
        SAVSection_Pollution* section = reader.readStruct<SAVSection_Pollution>(0);
        if (!section)
            break;

        if (!reader.readBlob(section->tilePollution, &m_tile_pollution))
            break;

        succeeded = true;
        break;
    }

    return succeeded;
}
