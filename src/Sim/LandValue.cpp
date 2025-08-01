/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "LandValue.h"
#include "../save_file.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Sim/City.h>
#include <Sim/Effect.h>

void initLandValueLayer(LandValueLayer* layer, City* city, MemoryArena* gameArena)
{
    initSectorGrid(&layer->sectors, gameArena, city->bounds.size(), 16, 8);

    layer->tileLandValue = gameArena->allocate_array_2d<u8>(city->bounds.size());
    fill<u8>(&layer->tileLandValue, 0);

    layer->tileBuildingContributions = gameArena->allocate_array_2d<s16>(city->bounds.size());
    fill<s16>(&layer->tileBuildingContributions, 0);

    initDirtyRects(&layer->dirtyRects, gameArena, maxLandValueEffectDistance, city->bounds);
}

void markLandValueLayerDirty(LandValueLayer* layer, Rect2I bounds)
{
    markRectAsDirty(&layer->dirtyRects, bounds);
}

void updateLandValueLayer(City* city, LandValueLayer* layer)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    if (isDirty(&layer->dirtyRects)) {
        {
            DEBUG_BLOCK_T("updateLandValueLayer: building effects", DebugCodeDataTag::Simulation);

            // Recalculate the building contributions
            for (auto rectIt = layer->dirtyRects.rects.iterate();
                rectIt.hasNext();
                rectIt.next()) {
                Rect2I dirtyRect = rectIt.getValue();

                fillRegion<s16>(&layer->tileBuildingContributions, dirtyRect, 0);

                ChunkedArray<Building*> contributingBuildings = findBuildingsOverlappingArea(city, dirtyRect.expanded(maxLandValueEffectDistance));
                for (auto buildingIt = contributingBuildings.iterate();
                    buildingIt.hasNext();
                    buildingIt.next()) {
                    Building* building = buildingIt.getValue();
                    BuildingDef* def = getBuildingDef(building);
                    if (def->landValueEffect.has_effect()) {
                        def->landValueEffect.apply(layer->tileBuildingContributions, dirtyRect, building->footprint.centre(), EffectType::Add);
                    }
                }

                //
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
                        s16 originalValue = layer->tileBuildingContributions.get(x, y);
                        s16 newValue = clamp<s16>(originalValue, -255, 255);
                        layer->tileBuildingContributions.set(x, y, newValue);
                    }
                }
            }
        }

        clearDirtyRects(&layer->dirtyRects);
    }

    // Recalculate overall value
    {
        DEBUG_BLOCK_T("updateLandValueLayer: overall calculation", DebugCodeDataTag::Simulation);

        for (s32 i = 0; i < layer->sectors.sectorsToUpdatePerTick; i++) {
            BasicSector* sector = getNextSector(&layer->sectors);

            for (s32 y = sector->bounds.y(); y < sector->bounds.y() + sector->bounds.height(); y++) {
                for (s32 x = sector->bounds.x(); x < sector->bounds.x() + sector->bounds.width(); x++) {
                    // Right now, we have very little to base this on!
                    // This explains how SC3K does it: http://www.sc3000.com/knowledge/showarticle.cfm?id=1132
                    // (However, apparently SC3K has an overflow bug with land value, so ehhhhhh...)
                    // -> Maybe we should use a larger int or even a float for the land value. Memory is cheap!
                    //
                    // Anyway... being near water, and being near forest are positive.
                    // Pollution, crime, good service coverage, and building effects all play their part.
                    // So does terrain height if we ever implement non-flat terrain!
                    //
                    // Also, global effects can influence land value. AKA, is the city nice to live in?
                    //
                    // At some point it'd probably be sensible to cache some of the factors in intermediate arrays,
                    // eg the effects of buildings, or distance to water, so we don't have to do a search for them for each tile.

                    float landValue = 0.1f;

                    // Waterfront = valuable
                    s32 distanceToWater = getDistanceToWaterAt(city, x, y);
                    if (distanceToWater < 10) {
                        landValue += (10 - distanceToWater) * 0.1f * 0.25f;
                    }

                    // Building effects
                    float buildingEffect = layer->tileBuildingContributions.get(x, y) / 255.0f;
                    landValue += buildingEffect;

                    // Fire protection = good
                    float fireProtection = getFireProtectionPercentAt(city, x, y);
                    landValue += fireProtection * 0.2f;

                    // Police protection = good
                    float policeCoverage = getPoliceCoveragePercentAt(city, x, y);
                    landValue += policeCoverage * 0.2f;

                    // Pollution = bad
                    float pollutionEffect = getPollutionPercentAt(city, x, y) * 0.1f;
                    landValue -= pollutionEffect;

                    layer->tileLandValue.set(x, y, clamp01AndMap_u8(landValue));
                }
            }
        }
    }
}

float getLandValuePercentAt(City* city, s32 x, s32 y)
{
    return city->landValueLayer.tileLandValue.get(x, y) / 255.0f;
}

void saveLandValueLayer(LandValueLayer* layer, BinaryFileWriter* writer)
{
    writer->startSection<SAVSection_LandValue>(SAV_LANDVALUE_ID, SAV_LANDVALUE_VERSION);
    SAVSection_LandValue landValueSection = {};

    // Tile land value
    landValueSection.tileLandValue = writer->appendBlob(&layer->tileLandValue, FileBlobCompressionScheme::RLE_S8);

    writer->endSection<SAVSection_LandValue>(&landValueSection);
}

bool loadLandValueLayer(LandValueLayer* layer, City*, BinaryFileReader* reader)
{
    bool succeeded = false;
    while (reader->startSection(SAV_LANDVALUE_ID, SAV_LANDVALUE_VERSION)) {
        SAVSection_LandValue* section = reader->readStruct<SAVSection_LandValue>(0);
        if (!section)
            break;

        if (!reader->readBlob(section->tileLandValue, &layer->tileLandValue))
            break;

        succeeded = true;
        break;
    }

    return succeeded;
}
