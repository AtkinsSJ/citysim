/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Transport.h"
#include "../save_file.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Sim/City.h>
#include <UI/Panel.h>

void initTransportLayer(TransportLayer* layer, City* city, MemoryArena* gameArena)
{
    layer->tileTransportTypes = gameArena->allocate_array_2d<Flags<TransportType>>(city->bounds.size());

    layer->transportMaxDistance = 8;
    initDirtyRects(&layer->dirtyRects, gameArena, layer->transportMaxDistance, city->bounds);

    for (auto type : enum_values<TransportType>()) {
        layer->tileTransportDistance[type] = gameArena->allocate_array_2d<u8>(city->bounds.size());
        fill<u8>(&layer->tileTransportDistance[type], 255);
    }
}

void updateTransportLayer(City* city, TransportLayer* layer)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    if (isDirty(&layer->dirtyRects)) {
        // Calculate transport types on each tile
        // So, I have two ideas about this:
        // 1: memset the area to 0 and then iterate through all the buildings in that area, applying their transport types
        // or 2: go tile-by-tile within the area, and see if a building exists or not, and set the transport types from that.
        // I'm not sure which is better? Right now a memset is easy because we're doing the entire map, but
        // that won't be the case for long and memsetting a rectangle within the city is going to be way fiddlier.
        // Also, there's a performance cost to gathering all the buildings up in that area into an array for iterating.
        // So, I think #2 is the better option, but I should test that later if it becomes expensive performance-wise.
        // - Sam, 28/08/2019

        for (auto it = layer->dirtyRects.rects.iterate();
            it.hasNext();
            it.next()) {
            Rect2I dirtyRect = it.getValue();

            // Transport types on tile, based on what buildings are present
            for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.height(); y++) {
                for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.width(); x++) {
                    Building* building = getBuildingAt(city, x, y);
                    if (building != nullptr) {
                        BuildingDef* def = getBuildingDef(building->typeID);
                        layer->tileTransportTypes.set(x, y, def->transportTypes);
                    } else {
                        layer->tileTransportTypes.set(x, y, {});
                    }
                }
            }

            // Clear the surrounding "distance to road" stuff from the rectangle
            for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.height(); y++) {
                for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.width(); x++) {
                    for (auto type : enum_values<TransportType>()) {
                        u8 distance = doesTileHaveTransport(city, x, y, type) ? 0 : 255;
                        layer->tileTransportDistance[type].set(x, y, distance);
                    }
                }
            }
        }

        // Transport distance recalculation
        for (auto type : enum_values<TransportType>()) {
            updateDistances(&layer->tileTransportDistance[type], &layer->dirtyRects, layer->transportMaxDistance);
        }

        clearDirtyRects(&layer->dirtyRects);
    }
}

void markTransportLayerDirty(TransportLayer* layer, Rect2I bounds)
{
    markRectAsDirty(&layer->dirtyRects, bounds);
}

bool doesTileHaveTransport(City* city, s32 x, s32 y, TransportType type)
{
    if (!tileExists(city, x, y))
        return false;

    return city->transportLayer.tileTransportTypes.get(x, y).has(type);
}

bool doesTileHaveTransport(City* city, s32 x, s32 y, Flags<TransportType> types)
{
    if (!tileExists(city, x, y))
        return false;

    return city->transportLayer.tileTransportTypes.get(x, y).has_any(types);
}

void addTransportToTile(City* city, s32 x, s32 y, TransportType type)
{
    city->transportLayer.tileTransportTypes.get(x, y).add(type);
}

void addTransportToTile(City* city, s32 x, s32 y, Flags<TransportType> types)
{
    city->transportLayer.tileTransportTypes.get(x, y).add_all(types);
}

s32 getDistanceToTransport(City* city, s32 x, s32 y, TransportType type)
{
    return city->transportLayer.tileTransportDistance[type].get(x, y);
}

void debugInspectTransport(UI::Panel* panel, City* city, s32 x, s32 y)
{
    panel->addLabel("*** TRANSPORT INFO ***"_s);

    // Transport
    for (auto type : enum_values<TransportType>()) {
        panel->addLabel(myprintf("Distance to transport #{0}: {1}"_s, { formatInt(type), formatInt(getDistanceToTransport(city, x, y, type)) }));
    }
}

void saveTransportLayer(TransportLayer*, BinaryFileWriter* writer)
{
    writer->startSection<SAVSection_Transport>(SAV_TRANSPORT_ID, SAV_TRANSPORT_VERSION);
    SAVSection_Transport transportSection = {};

    writer->endSection<SAVSection_Transport>(&transportSection);
}

bool loadTransportLayer(TransportLayer*, City*, BinaryFileReader* reader)
{
    bool succeeded = false;
    while (reader->startSection(SAV_TRANSPORT_ID, SAV_TRANSPORT_VERSION)) {
        SAVSection_Transport* section = reader->readStruct<SAVSection_Transport>(0);
        if (!section)
            break;

        // TODO: Implement Transport!

        succeeded = true;
        break;
    }

    return succeeded;
}
