/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Fire.h"
#include "../AppState.h"
#include "../save_file.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Sim/City.h>
#include <Sim/Effect.h>
#include <UI/Panel.h>
#include <Util/Random.h>

void initFireLayer(FireLayer* layer, City* city, MemoryArena* gameArena)
{
    layer->fundingLevel = 1.0f;

    layer->maxFireRadius = 4;
    layer->tileFireProximityEffect = gameArena->allocate_array_2d<u16>(city->bounds.w, city->bounds.h);
    fill<u16>(&layer->tileFireProximityEffect, 0);

    layer->tileTotalFireRisk = gameArena->allocate_array_2d<u8>(city->bounds.w, city->bounds.h);
    fill<u8>(&layer->tileTotalFireRisk, 0);
    layer->tileFireProtection = gameArena->allocate_array_2d<u8>(city->bounds.w, city->bounds.h);
    fill<u8>(&layer->tileFireProtection, 0);
    layer->tileOverallFireRisk = gameArena->allocate_array_2d<u8>(city->bounds.w, city->bounds.h);
    fill<u8>(&layer->tileOverallFireRisk, 0);

    initChunkedArray(&layer->fireProtectionBuildings, &city->buildingRefsChunkPool);

    initDirtyRects(&layer->dirtyRects, gameArena, layer->maxFireRadius, city->bounds);

    layer->activeFireCount = 0;
    initChunkPool(&layer->firePool, gameArena, 64);
    initSectorGrid(&layer->sectors, gameArena, city->bounds.w, city->bounds.h, 16, 8);
    for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&layer->sectors); sectorIndex++) {
        FireSector* sector = &layer->sectors.sectors[sectorIndex];

        initChunkedArray(&sector->activeFires, &layer->firePool);
    }
}

void markFireLayerDirty(FireLayer* layer, Rect2I bounds)
{
    markRectAsDirty(&layer->dirtyRects, bounds);
}

void updateFireLayer(City* city, FireLayer* layer)
{
    DEBUG_FUNCTION_T(DebugCodeDataTag::Simulation);

    if (isDirty(&layer->dirtyRects)) {
        DEBUG_BLOCK_T("updateFireLayer: building effects", DebugCodeDataTag::Simulation);

        // Recalculate fire distances
        for (auto rectIt = layer->dirtyRects.rects.iterate();
            rectIt.hasNext();
            rectIt.next()) {
            Rect2I dirtyRect = rectIt.getValue();
            fillRegion<u16>(&layer->tileFireProximityEffect, dirtyRect, 0);

            Rect2I expandedRect = expand(dirtyRect, layer->maxFireRadius);
            Rect2I affectedSectors = getSectorsCovered(&layer->sectors, expandedRect);

            for (s32 sy = affectedSectors.y; sy < affectedSectors.y + affectedSectors.h; sy++) {
                for (s32 sx = affectedSectors.x; sx < affectedSectors.x + affectedSectors.w; sx++) {
                    FireSector* sector = getSector(&layer->sectors, sx, sy);

                    for (auto it = sector->activeFires.iterate(); it.hasNext(); it.next()) {
                        Fire* fire = it.get();
                        if (expandedRect.contains(fire->pos)) {
                            // TODO: Different "strengths" of fire should have different effects
                            EffectRadius fireEffect { layer->maxFireRadius, 255, 20 };
                            fireEffect.apply(layer->tileFireProximityEffect, dirtyRect, v2(fire->pos), EffectType::Add);
                        }
                    }
                }
            }
        }

        clearDirtyRects(&layer->dirtyRects);
    }

    {
        DEBUG_BLOCK_T("updateFireLayer: overall calculation", DebugCodeDataTag::Simulation);

        for (s32 i = 0; i < layer->sectors.sectorsToUpdatePerTick; i++) {
            FireSector* sector = getNextSector(&layer->sectors);

            {
                DEBUG_BLOCK_T("updateFireLayer: building fire protection", DebugCodeDataTag::Simulation);
                // Building fire protection
                fillRegion<u8>(&layer->tileFireProtection, sector->bounds, 0);
                for (auto it = layer->fireProtectionBuildings.iterate(); it.hasNext(); it.next()) {
                    Building* building = getBuilding(city, it.getValue());
                    if (building != nullptr) {
                        BuildingDef* def = getBuildingDef(building);

                        float effectiveness = layer->fundingLevel;

                        if (!buildingHasPower(building)) {
                            effectiveness *= 0.4f; // @Balance
                        }

                        def->fireProtection.apply(layer->tileFireProtection, sector->bounds, building->footprint.centre(), EffectType::Max, effectiveness);
                    }
                }
            }

            for (s32 y = sector->bounds.y; y < sector->bounds.y + sector->bounds.h; y++) {
                for (s32 x = sector->bounds.x; x < sector->bounds.x + sector->bounds.w; x++) {
                    float tileFireRisk = 0.0f;

                    Building* building = getBuildingAt(city, x, y);
                    if (building) {
                        BuildingDef* def = getBuildingDef(building);
                        tileFireRisk += def->fireRisk;
                    }

                    // Being near a fire has a HUGE risk!
                    // TODO: Balance this! Currently it's a multiplier on the base building risk,
                    // because if we just ADD a value, then we get fire risk on empty tiles.
                    // But, the balance is definitely off.
                    u16 fireProximityEffect = layer->tileFireProximityEffect.get(x, y);
                    if (fireProximityEffect > 0) {
                        tileFireRisk += (tileFireRisk * 4.0f * clamp01(fireProximityEffect / 255.0f));
                    }

                    u8 totalRisk = clamp01AndMap_u8(tileFireRisk);
                    layer->tileTotalFireRisk.set(x, y, totalRisk);

                    // TODO: Balance this! It feels over-powered, even at 50%.
                    // Possibly fire stations shouldn't affect risk from active fires?
                    float protectionPercent = layer->tileFireProtection.get(x, y) * 0.01f;
                    u8 result = lerp<u8>(totalRisk, 0, protectionPercent * 0.5f);
                    layer->tileOverallFireRisk.set(x, y, result);
                }
            }
        }
    }
}

Indexed<Fire*> findFireAt(City* city, s32 x, s32 y)
{
    FireLayer* layer = &city->fireLayer;

    Indexed<Fire*> result = makeNullIndexedValue<Fire*>();

    if (layer->activeFireCount > 0) {
        FireSector* sector = getSectorAtTilePos(&layer->sectors, x, y);
        result = sector->activeFires.findFirst([=](Fire* fire) { return fire->pos.x == x && fire->pos.y == y; });
    }

    return result;
}

bool doesAreaContainFire(City* city, Rect2I bounds)
{
    FireLayer* layer = &city->fireLayer;
    bool foundFire = false;

    Rect2I footprintSectors = getSectorsCovered(&layer->sectors, bounds);
    for (s32 sy = footprintSectors.y;
        sy < footprintSectors.y + footprintSectors.h && !foundFire;
        sy++) {
        for (s32 sx = footprintSectors.x;
            sx < footprintSectors.x + footprintSectors.w && !foundFire;
            sx++) {
            FireSector* sector = getSector(&layer->sectors, sx, sy);
            for (auto it = sector->activeFires.iterate(); it.hasNext(); it.next()) {
                Fire* fire = it.get();

                if (bounds.contains(fire->pos)) {
                    foundFire = true;
                    break;
                }
            }
        }
    }

    return foundFire;
}

void startFireAt(City* city, s32 x, s32 y)
{
    Indexed<Fire*> existingFire = findFireAt(city, x, y);
    if (existingFire.value != nullptr) {
        // Fire already exists!
        // TODO: make the fire stronger?
        logWarn("Fire at {0},{1} already exists!"_s, { formatInt(x), formatInt(y) });
    } else {
        addFireRaw(city, x, y, getCurrentTimestamp());
    }
}

void addFireRaw(City* city, s32 x, s32 y, GameTimestamp startDate)
{
    // NB: We don't care if the fire already exists. startFireAt() already checks for this case.

    FireLayer* layer = &city->fireLayer;

    FireSector* sector = getSectorAtTilePos(&layer->sectors, x, y);

    Fire* fire = sector->activeFires.appendBlank();

    fire->pos = v2i(x, y);
    fire->startDate = startDate;
    fire->entity = addEntity(city, Entity::Type::Fire, fire);
    // TODO: Probably most of this wants to be moved into addEntity()
    fire->entity->bounds = { x, y, 1, 1 };
    fire->entity->sprite = getSpriteRef("e_fire_1x1"_s, AppState::the().cosmeticRandom->next());

    layer->activeFireCount++;

    markRectAsDirty(&layer->dirtyRects, { x, y, 1, 1 });
}

void updateFire(City* /*city*/, Fire* /*fire*/)
{
    // TODO: Burn baby burn, disco inferno
    // NB: This isn't actually called from anywhere right now.
}

void removeFireAt(City* city, s32 x, s32 y)
{
    FireLayer* layer = &city->fireLayer;

    FireSector* sectorAtPosition = getSectorAtTilePos(&layer->sectors, x, y);
    Indexed<Fire*> fireAtPosition = sectorAtPosition->activeFires.findFirst([=](Fire* fire) { return fire->pos.x == x && fire->pos.y == y; });

    if (fireAtPosition.value != nullptr) {
        // Remove it!
        removeEntity(city, fireAtPosition.value->entity);
        sectorAtPosition->activeFires.removeIndex(fireAtPosition.index);
        layer->activeFireCount--;

        markRectAsDirty(&layer->dirtyRects, { x, y, 1, 1 });
    }
}

void notifyNewBuilding(FireLayer* layer, BuildingDef* def, Building* building)
{
    if (def->fireProtection.has_effect()) {
        layer->fireProtectionBuildings.append(getReferenceTo(building));
    }
}

void notifyBuildingDemolished(FireLayer* layer, BuildingDef* def, Building* building)
{
    if (def->fireProtection.has_effect()) {
        bool success = layer->fireProtectionBuildings.findAndRemove(getReferenceTo(building));
        ASSERT(success);
    }
}

u8 getFireRiskAt(City* city, s32 x, s32 y)
{
    return city->fireLayer.tileTotalFireRisk.get(x, y);
}

float getFireProtectionPercentAt(City* city, s32 x, s32 y)
{
    return city->fireLayer.tileFireProtection.get(x, y) * 0.01f;
}

void debugInspectFire(UI::Panel* panel, City* city, s32 x, s32 y)
{
    FireLayer* layer = &city->fireLayer;

    panel->addLabel("*** FIRE INFO ***"_s);

    panel->addLabel(myprintf("There are {0} fire protection buildings and {1} active fires in the city."_s, { formatInt(layer->fireProtectionBuildings.count), formatInt(layer->activeFireCount) }));

    Building* buildingAtPos = getBuildingAt(city, x, y);
    float buildingFireRisk = 100.0f * ((buildingAtPos == nullptr) ? 0.0f : getBuildingDef(buildingAtPos)->fireRisk);

    panel->addLabel(myprintf("Fire risk: {0}, from:\n- Building: {1}%\n- Nearby fires: {2}"_s, {
                                                                                                   formatInt(getFireRiskAt(city, x, y)),
                                                                                                   formatFloat(buildingFireRisk, 1),
                                                                                                   formatInt(layer->tileFireProximityEffect.get(x, y)),
                                                                                               }));

    panel->addLabel(myprintf("Fire protection: {0}%"_s, { formatFloat(getFireProtectionPercentAt(city, x, y) * 100.0f, 0) }));

    panel->addLabel(myprintf("Resulting chance of fire: {0}%"_s, { formatFloat(layer->tileOverallFireRisk.get(x, y) / 2.55f, 1) }));
}

void saveFireLayer(FireLayer* layer, BinaryFileWriter* writer)
{
    writer->startSection<SAVSection_Fire>(SAV_FIRE_ID, SAV_FIRE_VERSION);
    SAVSection_Fire fireSection = {};

    // Active fires
    fireSection.activeFireCount = layer->activeFireCount;
    Array<SAVFire> tempFires = writer->arena->allocate_array<SAVFire>(fireSection.activeFireCount);
    // ughhhh I have to iterate the sectors to get this information!
    for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&layer->sectors); sectorIndex++) {
        FireSector* sector = getSectorByIndex(&layer->sectors, sectorIndex);

        for (auto it = sector->activeFires.iterate(); it.hasNext(); it.next()) {
            Fire* fire = it.get();
            SAVFire* savFire = tempFires.append();
            *savFire = {};
            savFire->x = (u16)fire->pos.x;
            savFire->y = (u16)fire->pos.y;
            savFire->startDate = fire->startDate;
        }
    }
    fireSection.activeFires = writer->appendArray<SAVFire>(tempFires);

    writer->endSection<SAVSection_Fire>(&fireSection);
}

bool loadFireLayer(FireLayer* layer, City* city, BinaryFileReader* reader)
{
    bool succeeded = false;
    while (reader->startSection(SAV_FIRE_ID, SAV_FIRE_VERSION)) {
        SAVSection_Fire* section = reader->readStruct<SAVSection_Fire>(0);
        if (!section)
            break;

        // Active fires
        Array<SAVFire> tempFires = reader->arena->allocate_array<SAVFire>(section->activeFireCount);
        if (!reader->readArray(section->activeFires, &tempFires))
            break;
        for (u32 activeFireIndex = 0;
            activeFireIndex < section->activeFireCount;
            activeFireIndex++) {
            SAVFire* savFire = &tempFires[activeFireIndex];

            addFireRaw(city, savFire->x, savFire->y, savFire->startDate);
        }
        ASSERT((u32)layer->activeFireCount == section->activeFireCount);

        succeeded = true;
        break;
    }

    return succeeded;
}
