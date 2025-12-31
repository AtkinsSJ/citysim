/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "save_file.h"
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
            metaSection.population = getTotalResidents(city);
            metaSection.jobs = getTotalJobs(city);

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
        saveTerrainLayer(&city->terrainLayer, &writer);

        // Buildings
        saveBuildings(city, &writer);

        // Layers
        saveBudgetLayer(&city->budgetLayer, &writer);
        saveCrimeLayer(&city->crimeLayer, &writer);
        saveEducationLayer(&city->educationLayer, &writer);
        saveFireLayer(&city->fireLayer, &writer);
        saveHealthLayer(&city->healthLayer, &writer);
        saveLandValueLayer(&city->landValueLayer, &writer);
        savePollutionLayer(&city->pollutionLayer, &writer);
        saveTransportLayer(&city->transportLayer, &writer);
        saveZoneLayer(&city->zoneLayer, &writer);

        succeeded = writer.outputToFile(file);
    }

    return succeeded;
}

bool loadSaveFile(FileHandle* file, GameState* gameState)
{
    // So... I'm not really sure how to signal success, honestly.
    // I suppose the process ouytside of this function is:
    // - User confirms to load a city.
    // - Existing city, if any, is discarded.
    // - This function is called.
    // - If it fails, discard the city, else it's in memory.
    // So, if loading fails, then no city will be in memory, regardless of whether one was
    // before the loading was attempted! I think that makes the most sense.
    // Another option would be to load into a second City struct, and then swap it if it
    // successfully loads... but that makes a bunch of memory-management more complicated.
    // This way, we only ever have one City in memory so we can clean up easily.

    // For now, reading the whole thing into memory and then processing it is simpler.
    // However, it's wasteful memory-wise, so if save files get big we might want to
    // read the file a bit at a time. @Size

    bool succeeded = false;

    City* city = &gameState->city;

    BinaryFileReader reader = readBinaryFile(file, SAV_FILE_ID, &temp_arena());
    // This doesn't actually loop, we're just using a `while` so we can break out of it
    while (reader.isValidFile) {
        // META
        bool readMeta = reader.startSection(SAV_META_ID, SAV_META_VERSION);
        if (readMeta) {
            SAVSection_Meta* meta = reader.readStruct<SAVSection_Meta>(0);

            String cityName = reader.readString(meta->cityName);
            String playerName = reader.readString(meta->playerName);
            initCity(&gameState->arena, city, meta->cityWidth, meta->cityHeight, cityName, playerName, meta->funds);

            // Clock
            initGameClock(&gameState->gameClock, meta->currentDate, meta->timeWithinDay);

            // Camera
            auto& world_camera = the_renderer().world_camera();
            world_camera.set_position(v2(meta->cameraX, meta->cameraY));
            world_camera.set_zoom(meta->cameraZoom);
        } else
            break;

        if (!loadTerrainLayer(&city->terrainLayer, city, &reader))
            break;
        if (!loadBuildings(city, &reader))
            break;
        if (!loadZoneLayer(&city->zoneLayer, city, &reader))
            break;
        if (!loadCrimeLayer(&city->crimeLayer, city, &reader))
            break;
        if (!loadEducationLayer(&city->educationLayer, city, &reader))
            break;
        if (!loadFireLayer(&city->fireLayer, city, &reader))
            break;
        if (!loadHealthLayer(&city->healthLayer, city, &reader))
            break;
        if (!loadLandValueLayer(&city->landValueLayer, city, &reader))
            break;
        if (!loadPollutionLayer(&city->pollutionLayer, city, &reader))
            break;
        if (!loadTransportLayer(&city->transportLayer, city, &reader))
            break;
        if (!loadBudgetLayer(&city->budgetLayer, city, &reader))
            break;

        // And we're done!
        succeeded = true;
        break;
    }

    return succeeded;
}
