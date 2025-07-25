/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "health.h"
#include "binary_file_reader.h"
#include "binary_file_writer.h"
#include "city.h"
#include "save_file.h"

float getHealthCoveragePercentAt(City* city, s32 x, s32 y)
{
    return city->healthLayer.tileHealthCoverage.get(x, y) * 0.01f;
}

void initHealthLayer(HealthLayer* layer, City* city, MemoryArena* gameArena)
{
    initSectorGrid(&layer->sectors, gameArena, city->bounds.w, city->bounds.h, 16, 8);

    initDirtyRects(&layer->dirtyRects, gameArena, maxLandValueEffectDistance, city->bounds);

    layer->tileHealthCoverage = gameArena->allocate_array_2d<u8>(city->bounds.w, city->bounds.h);
    fill<u8>(&layer->tileHealthCoverage, 0);

    initChunkedArray(&layer->healthBuildings, &city->buildingRefsChunkPool);

    layer->fundingLevel = 1.0f;
}

void updateHealthLayer(City* city, HealthLayer* layer)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    if (isDirty(&layer->dirtyRects)) {
        DEBUG_BLOCK_T("updateHealthLayer: dirty rects", DebugCodeDataTag::Simulation);

        // TODO: do we actually need dirty rects? I can't think of anything, unless we move the "register building" stuff to that.

        clearDirtyRects(&layer->dirtyRects);
    }

    {
        DEBUG_BLOCK_T("updateHealthLayer: sector updates", DebugCodeDataTag::Simulation);

        for (s32 i = 0; i < layer->sectors.sectorsToUpdatePerTick; i++) {
            BasicSector* sector = getNextSector(&layer->sectors);

            {
                DEBUG_BLOCK_T("updateHealthLayer: building health coverage", DebugCodeDataTag::Simulation);
                fillRegion<u8>(&layer->tileHealthCoverage, sector->bounds, 0);
                for (auto it = layer->healthBuildings.iterate(); it.hasNext(); it.next()) {
                    Building* building = getBuilding(city, it.getValue());
                    if (building != nullptr) {
                        BuildingDef* def = getBuildingDef(building);

                        // Budget
                        float effectiveness = layer->fundingLevel;

                        // Power
                        if (!buildingHasPower(building)) {
                            effectiveness *= 0.4f; // @Balance
                        }

                        // TODO: Water

                        // TODO: Overcrowding

                        applyEffect(&def->healthEffect, centreOf(building->footprint), EffectType::Max, &layer->tileHealthCoverage, sector->bounds, effectiveness);
                    }
                }
            }
        }
    }
}

void markHealthLayerDirty(HealthLayer* layer, Rect2I bounds)
{
    markRectAsDirty(&layer->dirtyRects, bounds);
}

void notifyNewBuilding(HealthLayer* layer, BuildingDef* def, Building* building)
{
    if (hasEffect(&def->healthEffect)) {
        layer->healthBuildings.append(getReferenceTo(building));
    }
}

void notifyBuildingDemolished(HealthLayer* layer, BuildingDef* def, Building* building)
{
    if (hasEffect(&def->healthEffect)) {
        bool success = layer->healthBuildings.findAndRemove(getReferenceTo(building));
        ASSERT(success);
    }
}

void saveHealthLayer(HealthLayer* /*layer*/, struct BinaryFileWriter* writer)
{
    writer->startSection<SAVSection_Health>(SAV_HEALTH_ID, SAV_HEALTH_VERSION);
    SAVSection_Health healthSection = {};

    writer->endSection<SAVSection_Health>(&healthSection);
}

bool loadHealthLayer(HealthLayer* /*layer*/, City* /*city*/, struct BinaryFileReader* reader)
{
    bool succeeded = false;
    while (reader->startSection(SAV_HEALTH_ID, SAV_HEALTH_VERSION)) {
        SAVSection_Health* section = reader->readStruct<SAVSection_Health>(0);
        if (!section)
            break;

        succeeded = true;
        break;
    }

    return succeeded;
}
