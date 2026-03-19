/*
 * Copyright (c) 2019-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "SaveFile.h"
#include <Gfx/Renderer.h>
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Sim/City.h>
#include <Sim/Game.h>
#include <Sim/Terrain.h>
#include <Sim/Zone.h>

bool writeSaveFile(FileHandle* file, GameState* gameState)
{
    bool succeeded = file->isOpen;

    if (succeeded) {
        City* city = &gameState->city;

        BinaryFileWriter writer = startWritingFile(SAV_FILE_ID, SAV_VERSION, &temp_arena());

        // Prepare the TOC
        writer.addTOCEntry(SAV_META_ID);
        writer.addTOCEntry(SAV_BUDGET_ID);
        writer.addTOCEntry(SAV_BUILDING_ID);
        writer.addTOCEntry(SAV_CRIME_ID);
        writer.addTOCEntry(SAV_EDUCATION_ID);
        writer.addTOCEntry(SAV_FIRE_ID);
        writer.addTOCEntry(SAV_HEALTH_ID);
        writer.addTOCEntry(SAV_LANDVALUE_ID);
        writer.addTOCEntry(SAV_POLLUTION_ID);
        writer.addTOCEntry(SAV_TERRAIN_ID);
        writer.addTOCEntry(SAV_TRANSPORT_ID);
        writer.addTOCEntry(SAV_ZONE_ID);

        // Meta
        {
            writer.startSection<SAVSection_Meta>(SAV_META_ID, SAV_META_VERSION);
            SAVSection_Meta metaSection = {};

            metaSection.saveTimestamp = get_current_unix_timestamp();
            metaSection.cityWidth = (u16)city->bounds.width();
            metaSection.cityHeight = (u16)city->bounds.height();
            metaSection.funds = city->funds;
            metaSection.population = city->zoneLayer.total_residents();
            metaSection.jobs = city->zoneLayer.total_jobs();

            metaSection.cityName = writer.append_string(city->name);
            metaSection.playerName = writer.append_string(city->playerName);

            // Clock
            GameClock* clock = &gameState->gameClock;
            metaSection.currentDate = clock->currentDay;
            metaSection.timeWithinDay = clock->timeWithinDay;

            // Camera
            Camera& camera = the_renderer().world_camera();
            metaSection.cameraX = camera.position().x;
            metaSection.cameraY = camera.position().y;
            metaSection.cameraZoom = camera.zoom();

            writer.endSection(&metaSection);
        }

        // Terrain
        city->terrainLayer.save(writer);

        // Buildings
        city->save_buildings(&writer);

        // Layers
        city->budgetLayer.save(writer);
        city->crimeLayer.save(writer);
        city->educationLayer.save(writer);
        city->fireLayer.save(writer);
        city->healthLayer.save(writer);
        saveLandValueLayer(&city->landValueLayer, &writer);
        savePollutionLayer(&city->pollutionLayer, &writer);
        city->transportLayer.save(writer);
        city->zoneLayer.save(writer);

        succeeded = writer.outputToFile(file);
    }

    return succeeded;
}
