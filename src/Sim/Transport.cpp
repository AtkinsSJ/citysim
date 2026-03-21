/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Transport.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Menus/SaveFile.h>
#include <Sim/City.h>
#include <UI/Panel.h>

TransportLayer::TransportLayer(City& city, MemoryArena& arena)
    : m_dirty_rects(arena, city.bounds)
{
    m_tile_transport_types = arena.allocate_array_2d<Flags<TransportType>>(city.bounds.size());

    for (auto type : enum_values<TransportType>()) {
        m_tile_transport_distance[type] = arena.allocate_array_2d<u8>(city.bounds.size());
        m_tile_transport_distance[type].fill(255);
    }
}

void TransportLayer::update(City& city)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    if (m_dirty_rects.is_dirty()) {
        // Calculate transport types on each tile
        // So, I have two ideas about this:
        // 1: memset the area to 0 and then iterate through all the buildings in that area, applying their transport types
        // or 2: go tile-by-tile within the area, and see if a building exists or not, and set the transport types from that.
        // I'm not sure which is better? Right now a memset is easy because we're doing the entire map, but
        // that won't be the case for long and memsetting a rectangle within the city is going to be way fiddlier.
        // Also, there's a performance cost to gathering all the buildings up in that area into an array for iterating.
        // So, I think #2 is the better option, but I should test that later if it becomes expensive performance-wise.
        // - Sam, 28/08/2019

        for (auto it = m_dirty_rects.rects().iterate();
            it.hasNext();
            it.next()) {
            Rect2I dirtyRect = it.getValue();

            // Transport types on tile, based on what buildings are present
            for (s32 y = dirtyRect.y(); y < dirtyRect.y() + dirtyRect.height(); y++) {
                for (s32 x = dirtyRect.x(); x < dirtyRect.x() + dirtyRect.width(); x++) {
                    Building* building = city.get_building_at(x, y);
                    if (building != nullptr) {
                        BuildingDef* def = getBuildingDef(building);
                        m_tile_transport_types.set(x, y, def->transportTypes);
                    } else {
                        m_tile_transport_types.set(x, y, {});
                    }
                }
            }

            // Clear the surrounding "distance to road" stuff from the rectangle
            for (s32 y = dirtyRect.y(); y < dirtyRect.y() + dirtyRect.height(); y++) {
                for (s32 x = dirtyRect.x(); x < dirtyRect.x() + dirtyRect.width(); x++) {
                    for (auto type : enum_values<TransportType>()) {
                        u8 distance = tile_has_transport(x, y, type) ? 0 : 255;
                        m_tile_transport_distance[type].set(x, y, distance);
                    }
                }
            }
        }

        // Transport distance recalculation
        for (auto type : enum_values<TransportType>()) {
            updateDistances(&m_tile_transport_distance[type], &m_dirty_rects, m_transport_max_distance);
        }

        m_dirty_rects.clear();
    }
}

void TransportLayer::mark_dirty(Rect2I bounds)
{
    m_dirty_rects.mark_dirty(bounds.expanded(m_transport_max_distance));
}

bool TransportLayer::tile_has_transport(s32 x, s32 y, TransportType type) const
{
    return m_tile_transport_types.get(x, y).has(type);
}

bool TransportLayer::tile_has_transport(s32 x, s32 y, Flags<TransportType> types) const
{
    return m_tile_transport_types.get(x, y).has_any(types);
}

void TransportLayer::add_transport_to_tile(s32 x, s32 y, TransportType type)
{
    m_tile_transport_types.get(x, y).add(type);
}

void TransportLayer::add_transport_to_tile(s32 x, s32 y, Flags<TransportType> types)
{
    m_tile_transport_types.get(x, y).add_all(types);
}

s32 TransportLayer::distance_to_transport(s32 x, s32 y, TransportType type) const
{
    return m_tile_transport_distance[type].get(x, y);
}

void TransportLayer::debug_inspect(UI::Panel& panel, V2I tile_position)
{
    panel.addLabel("*** TRANSPORT INFO ***"_s);

    // Transport
    for (auto type : enum_values<TransportType>()) {
        panel.addLabel(myprintf("Distance to transport #{0}: {1}"_s, { formatInt(type), formatInt(distance_to_transport(tile_position.x, tile_position.y, type)) }));
    }
}

void TransportLayer::save(BinaryFileWriter& writer) const
{
    writer.startSection<SAVSection_Transport>(SAV_TRANSPORT_ID, SAV_TRANSPORT_VERSION);
    SAVSection_Transport transportSection = {};

    writer.endSection<SAVSection_Transport>(&transportSection);
}

bool TransportLayer::load(BinaryFileReader& reader)
{
    bool succeeded = false;
    while (reader.startSection(SAV_TRANSPORT_ID, SAV_TRANSPORT_VERSION)) {
        SAVSection_Transport* section = reader.readStruct<SAVSection_Transport>(0);
        if (!section)
            break;

        // TODO: Implement Transport!

        succeeded = true;
        break;
    }

    return succeeded;
}
