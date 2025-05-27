#pragma once

void initPollutionLayer(PollutionLayer* layer, City* city, MemoryArena* gameArena)
{
    layer->tilePollution = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
    fill<u8>(&layer->tilePollution, 0);

    layer->tileBuildingContributions = allocateArray2<s16>(gameArena, city->bounds.w, city->bounds.h);
    fill<s16>(&layer->tileBuildingContributions, 0);

    initDirtyRects(&layer->dirtyRects, gameArena, maxPollutionEffectDistance, city->bounds);
}

void updatePollutionLayer(City* city, PollutionLayer* layer)
{
    DEBUG_FUNCTION_T(DCDT_Simulation);

    if (isDirty(&layer->dirtyRects)) {
        // @Copypasta from updateLandValueLayer()
        {
            DEBUG_BLOCK_T("updatePollutionLayer: building effects", DCDT_Simulation);

            // Recalculate the building contributions
            for (auto rectIt = layer->dirtyRects.rects.iterate();
                rectIt.hasNext();
                rectIt.next()) {
                Rect2I dirtyRect = rectIt.getValue();

                fillRegion<s16>(&layer->tileBuildingContributions, dirtyRect, 0);

                ChunkedArray<Building*> contributingBuildings = findBuildingsOverlappingArea(city, expand(dirtyRect, maxLandValueEffectDistance), 0);
                for (auto buildingIt = contributingBuildings.iterate();
                    buildingIt.hasNext();
                    buildingIt.next()) {
                    Building* building = buildingIt.getValue();
                    BuildingDef* def = getBuildingDef(building);
                    if (hasEffect(&def->pollutionEffect)) {
                        applyEffect(&def->pollutionEffect, centreOf(building->footprint), Effect_Add, &layer->tileBuildingContributions, dirtyRect);
                    }
                }

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
                for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++) {
                    for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++) {
                        s16 originalValue = layer->tileBuildingContributions.get(x, y);
                        s16 newValue = clamp<s16>(originalValue, -255, 255);
                        layer->tileBuildingContributions.set(x, y, newValue);
                    }
                }
            }
        }

        // Recalculate overall value
        {
            DEBUG_BLOCK_T("updatePollutionLayer: combine", DCDT_Simulation);

            for (auto rectIt = layer->dirtyRects.rects.iterate();
                rectIt.hasNext();
                rectIt.next()) {
                Rect2I dirtyRect = rectIt.getValue();

                for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++) {
                    for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++) {
                        // This is really lazy right now.
                        // In future we might want to update things over time, but right now it's just
                        // a glorified copy from the tileBuildingContributions array.

                        s16 buildingContributions = layer->tileBuildingContributions.get(x, y);

                        u8 newValue = (u8)clamp<s16>(buildingContributions, 0, 255);

                        layer->tilePollution.set(x, y, newValue);
                    }
                }
            }
        }

        clearDirtyRects(&layer->dirtyRects);
    }
}

void markPollutionLayerDirty(PollutionLayer* layer, Rect2I bounds)
{
    markRectAsDirty(&layer->dirtyRects, bounds);
}

inline u8 getPollutionAt(City* city, s32 x, s32 y)
{
    return city->pollutionLayer.tilePollution.get(x, y);
}

inline f32 getPollutionPercentAt(City* city, s32 x, s32 y)
{
    return city->pollutionLayer.tilePollution.get(x, y) / 255.0f;
}

void savePollutionLayer(PollutionLayer* layer, struct BinaryFileWriter* writer)
{
    writer->startSection<SAVSection_Pollution>(SAV_POLLUTION_ID, SAV_POLLUTION_VERSION);
    SAVSection_Pollution pollutionSection = {};

    // Tile pollution
    pollutionSection.tilePollution = writer->appendBlob(&layer->tilePollution, Blob_RLE_S8);

    writer->endSection<SAVSection_Pollution>(&pollutionSection);
}

bool loadPollutionLayer(PollutionLayer* layer, City* /*city*/, struct BinaryFileReader* reader)
{
    bool succeeded = false;
    while (reader->startSection(SAV_POLLUTION_ID, SAV_POLLUTION_VERSION)) {
        SAVSection_Pollution* section = reader->readStruct<SAVSection_Pollution>(0);
        if (!section)
            break;

        if (!reader->readBlob(section->tilePollution, &layer->tilePollution))
            break;

        succeeded = true;
        break;
    }

    return succeeded;
}
