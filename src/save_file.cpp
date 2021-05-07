#pragma once

bool writeSaveFile(FileHandle *file, GameState *gameState)
{
	bool succeeded = file->isOpen;

	if (succeeded)
	{
		MemoryArena *arena = tempArena;

		City *city = &gameState->city;

		BinaryFileWriter writer = startWritingFile(SAV_FILE_ID, SAV_VERSION, arena);

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

			metaSection.saveTimestamp = getCurrentUnixTimestamp();
			metaSection.cityWidth     = (u16) city->bounds.w;
			metaSection.cityHeight    = (u16) city->bounds.h;
			metaSection.funds         = city->funds;
			metaSection.population    = getTotalResidents(city);
			metaSection.jobs          = getTotalJobs(city);

			metaSection.cityName   = writer.appendString(city->name);
			metaSection.playerName = writer.appendString(city->playerName);

			// Clock
			GameClock *clock = &gameState->gameClock;
			metaSection.currentDate = clock->currentDay;
			metaSection.timeWithinDay = clock->timeWithinDay;
			
			// Camera
			Camera *camera = &renderer->worldCamera;
			metaSection.cameraX = camera->pos.x;
			metaSection.cameraY = camera->pos.y;
			metaSection.cameraZoom = camera->zoom;

			writer.endSection(&metaSection);
		}

		// Terrain
		{
			writer.startSection<SAVSection_Terrain>(SAV_TERRAIN_ID, SAV_TERRAIN_VERSION);
			SAVSection_Terrain terrainSection = {};
			TerrainLayer *layer = &city->terrainLayer;

			// Terrain generation parameters
			terrainSection.terrainGenerationSeed = layer->terrainGenerationSeed;

			// Terrain types table
			s32 terrainDefCount = terrainCatalogue.terrainDefs.count - 1; // Skip the null def
			WriteBufferRange terrainTypeTableLoc = writer.reserveArray<SAVTerrainTypeEntry>(terrainDefCount);
			Array<SAVTerrainTypeEntry> terrainTypeTable = allocateArray<SAVTerrainTypeEntry>(arena, terrainDefCount);
			for (auto it = terrainCatalogue.terrainDefs.iterate(); it.hasNext(); it.next())
			{
				TerrainDef *def = it.get();
				if (def->typeID == 0) continue; // Skip the null terrain def!

				SAVTerrainTypeEntry *entry = terrainTypeTable.append();
				entry->typeID = def->typeID;
				entry->name = writer.appendString(def->name);
			}
			terrainSection.terrainTypeTable = writer.writeArray<SAVTerrainTypeEntry>(terrainTypeTable, terrainTypeTableLoc);

			// Tile terrain type (u8)
			terrainSection.tileTerrainType = writer.appendBlob(&layer->tileTerrainType, Blob_RLE_S8);

			// Tile height (u8)
			terrainSection.tileHeight = writer.appendBlob(&layer->tileHeight, Blob_RLE_S8);

			// Tile sprite offset (u8)
			terrainSection.tileSpriteOffset = writer.appendBlob(&layer->tileSpriteOffset, Blob_Uncompressed);

			writer.endSection<SAVSection_Terrain>(&terrainSection);
		}

		// Buildings
		{
			writer.startSection<SAVSection_Buildings>(SAV_BUILDING_ID, SAV_BUILDING_VERSION);
			SAVSection_Buildings buildingSection = {};

			// Building types table
			s32 buildingDefCount = buildingCatalogue.allBuildings.count - 1; // Skip the null def
			WriteBufferRange buildingTypeTableLoc = writer.reserveArray<SAVBuildingTypeEntry>(buildingDefCount);
			Array<SAVBuildingTypeEntry> buildingTypeTable = allocateArray<SAVBuildingTypeEntry>(arena, buildingDefCount);
			for (auto it = buildingCatalogue.allBuildings.iterate(); it.hasNext(); it.next())
			{
				BuildingDef *def = it.get();
				if (def->typeID == 0) continue; // Skip the null building def!

				SAVBuildingTypeEntry *entry = buildingTypeTable.append();
				entry->typeID = def->typeID;
				entry->name = writer.appendString(def->name);
			}
			buildingSection.buildingTypeTable = writer.writeArray<SAVBuildingTypeEntry>(buildingTypeTable, buildingTypeTableLoc);

			// Highest ID
			buildingSection.highestBuildingID = city->highestBuildingID;

			//
			// The buildings themselves!
			// I'm not sure how to actually do this... current thought is that the "core" building
			// data will be here, and that other stuff (eg, health/education of the occupants)
			// will be in the relevant layers' chunks. That seems the most extensible?
			// Another option would be to store the buildings here as a series of arrays, one
			// for each field, like we do for Terrain. That feels really messy though.
			// The tricky thing is that the Building struct in game feels likely to change a lot,
			// and we want the save format to change as little as possible... though that only
			// matters once the game is released (or near enough release that I start making
			// pre-made maps) so maybe it's not such a big issue?
			// Eh, going to just go ahead with a placeholder version, like the rest of this code!
			//
			// - Sam, 11/10/2019
			//
			buildingSection.buildingCount = city->buildings.count - 1; // Not the null building!

			SAVBuilding *tempBuildings = allocateMultiple<SAVBuilding>(arena, buildingSection.buildingCount);
			s32 tempBuildingIndex = 0;

			for (auto it = city->buildings.iterate(); it.hasNext(); it.next())
			{
				Building *building = it.get();
				if (building->id == 0) continue; // Skip the null building!

				SAVBuilding *sb = tempBuildings + tempBuildingIndex;
				*sb = {};
				sb->id = building->id;
				sb->typeID = building->typeID;
				sb->creationDate = building->creationDate;
				sb->x = (u16) building->footprint.x;
				sb->y = (u16) building->footprint.y;
				sb->w = (u16) building->footprint.w;
				sb->h = (u16) building->footprint.h;
				sb->spriteOffset = (u16) building->spriteOffset;
				sb->currentResidents = (u16) building->currentResidents;
				sb->currentJobs = (u16) building->currentJobs;
				sb->variantIndex = (u16) building->variantIndex;

				tempBuildingIndex++;
			}
			buildingSection.buildings = writer.appendBlob(buildingSection.buildingCount * sizeof(SAVBuilding), (u8*)tempBuildings, Blob_Uncompressed);

			writer.endSection<SAVSection_Buildings>(&buildingSection);
		}

		// Zones
		{
			ZoneLayer *layer = &city->zoneLayer;
			writer.startSection<SAVSection_Zone>(SAV_ZONE_ID, SAV_ZONE_VERSION);
			SAVSection_Zone zoneSection = {};

			// Tile zones
			zoneSection.tileZone = writer.appendBlob(&layer->tileZone, Blob_RLE_S8);

			writer.endSection<SAVSection_Zone>(&zoneSection);
		}

		// Budget
		{
			writer.startSection<SAVSection_Budget>(SAV_BUDGET_ID, SAV_BUDGET_VERSION);
			SAVSection_Budget budgetSection = {};
			// BudgetLayer *layer = &city->budgetLayer;

			writer.endSection<SAVSection_Budget>(&budgetSection);
		}

		// Crime
		{
			CrimeLayer *layer = &city->crimeLayer;
			writer.startSection<SAVSection_Crime>(SAV_CRIME_ID, SAV_CRIME_VERSION);
			SAVSection_Crime crimeSection = {};

			crimeSection.totalJailCapacity = layer->totalJailCapacity;
			crimeSection.occupiedJailCapacity = layer->occupiedJailCapacity;

			writer.endSection<SAVSection_Crime>(&crimeSection);
		}

		// Education
		{
			writer.startSection<SAVSection_Education>(SAV_EDUCATION_ID, SAV_EDUCATION_VERSION);
			SAVSection_Education educationSection = {};
			// EducationLayer *layer = &city->educationLayer;

			writer.endSection<SAVSection_Education>(&educationSection);
		}

		// Fire
		{
			writer.startSection<SAVSection_Fire>(SAV_FIRE_ID, SAV_FIRE_VERSION);
			SAVSection_Fire fireSection = {};
			FireLayer *layer = &city->fireLayer;

			// Active fires
			fireSection.activeFireCount = layer->activeFireCount;
			Array<SAVFire> tempFires = allocateArray<SAVFire>(arena, fireSection.activeFireCount);
			// ughhhh I have to iterate the sectors to get this information!
			for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&layer->sectors); sectorIndex++)
			{
				FireSector *sector = getSectorByIndex(&layer->sectors, sectorIndex);

				for (auto it = sector->activeFires.iterate(); it.hasNext(); it.next())
				{
					Fire *fire = it.get();
					SAVFire *savFire = tempFires.append();
					*savFire = {};
					savFire->x = (u16) fire->pos.x;
					savFire->y = (u16) fire->pos.y;
					savFire->startDate = fire->startDate;
				}
			}
			fireSection.activeFires = writer.appendArray<SAVFire>(tempFires);

			writer.endSection<SAVSection_Fire>(&fireSection);
		}

		// Health
		{
			writer.startSection<SAVSection_Health>(SAV_HEALTH_ID, SAV_HEALTH_VERSION);
			SAVSection_Health healthSection = {};
			// HealthLayer *layer = &city->healthLayer;

			writer.endSection<SAVSection_Health>(&healthSection);
		}

		// Land Value
		{
			writer.startSection<SAVSection_LandValue>(SAV_LANDVALUE_ID, SAV_LANDVALUE_VERSION);
			SAVSection_LandValue landValueSection = {};
			LandValueLayer *layer = &city->landValueLayer;

			// Tile land value
			landValueSection.tileLandValue = writer.appendBlob(&layer->tileLandValue, Blob_Uncompressed);

			writer.endSection<SAVSection_LandValue>(&landValueSection);
		}

		// Pollution
		{
			writer.startSection<SAVSection_Pollution>(SAV_POLLUTION_ID, SAV_POLLUTION_VERSION);
			SAVSection_Pollution pollutionSection = {};
			PollutionLayer *layer = &city->pollutionLayer;

			// Tile pollution
			pollutionSection.tilePollution = writer.appendBlob(&layer->tilePollution, Blob_Uncompressed);

			writer.endSection<SAVSection_Pollution>(&pollutionSection);
		}

		// Transport
		{
			writer.startSection<SAVSection_Transport>(SAV_TRANSPORT_ID, SAV_TRANSPORT_VERSION);
			SAVSection_Transport transportSection = {};
			// TransportLayer *layer = &city->transportLayer;

			writer.endSection<SAVSection_Transport>(&transportSection);
		}

		succeeded = writer.outputToFile(file);
	}

	return succeeded;
}

bool loadSaveFile(FileHandle *file, GameState *gameState)
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

	City *city = &gameState->city;
	MemoryArena *arena = tempArena;

	BinaryFileReader reader = readBinaryFile(file, SAV_FILE_ID, arena);
	// This doesn't actually loop, we're just using a `while` so we can break out of it
	while (reader.isValidFile)
	{
		s32 cityTileCount = 0;

		// META
		bool readMeta = reader.startSection(SAV_META_ID, SAV_META_VERSION);
		if (readMeta)
		{
			SAVSection_Meta *meta = reader.readStruct<SAVSection_Meta>(0);

			String cityName = reader.readString(meta->cityName);
			String playerName = reader.readString(meta->playerName);
			initCity(&gameState->gameArena, city, meta->cityWidth, meta->cityHeight, cityName, playerName, meta->funds);
			cityTileCount = city->bounds.w * city->bounds.h;

			// Clock
			initGameClock(&gameState->gameClock, meta->currentDate, meta->timeWithinDay);

			// Camera
			setCameraPos(&renderer->worldCamera, v2(meta->cameraX, meta->cameraY), meta->cameraZoom);
		}
		else break;

		// TERRAIN
		bool readTerrain = reader.startSection(SAV_TERRAIN_ID, SAV_TERRAIN_VERSION);
		if (readTerrain)
		{
			SAVSection_Terrain *terrain = reader.readStruct<SAVSection_Terrain>(0);
			TerrainLayer *layer = &city->terrainLayer;

			layer->terrainGenerationSeed = terrain->terrainGenerationSeed;

			// Map the file's terrain type IDs to the game's ones
			// NB: count+1 because the file won't save the null terrain, so we need to compensate
			Array<u8> oldTypeToNewType = allocateArray<u8>(arena, terrain->terrainTypeTable.count + 1, true);
			Array<SAVTerrainTypeEntry> terrainTypeTable = allocateArray<SAVTerrainTypeEntry>(arena, terrain->terrainTypeTable.count);
			if (!reader.readArray(terrain->terrainTypeTable, &terrainTypeTable)) break;
			for (s32 i=0; i < terrainTypeTable.count; i++)
			{
				SAVTerrainTypeEntry *entry = &terrainTypeTable[i];
				String terrainName = reader.readString(entry->name);
				oldTypeToNewType[entry->typeID] = findTerrainTypeByName(terrainName);
			}

			// Terrain type
			if (!reader.readBlob(terrain->tileTerrainType, &layer->tileTerrainType)) break;
			for (s32 y = 0; y < city->bounds.y; y++)
			{
				for (s32 x = 0; x < city->bounds.x; x++)
				{
					layer->tileTerrainType.set(x, y,  oldTypeToNewType[layer->tileTerrainType.get(x, y)]);
				}
			}

			// Terrain height
			if (!reader.readBlob(terrain->tileHeight, &layer->tileHeight)) break;

			// Sprite offset
			if (!reader.readBlob(terrain->tileSpriteOffset, &layer->tileSpriteOffset)) break;

			assignTerrainSprites(city, city->bounds);
		}
		else break;

		// BUILDINGS
		bool readBuildings = reader.startSection(SAV_BUILDING_ID, SAV_BUILDING_VERSION);
		if (readBuildings)
		{
			SAVSection_Buildings *cBuildings = reader.readStruct<SAVSection_Buildings>(0);

			city->highestBuildingID = cBuildings->highestBuildingID;

			// Map the file's building type IDs to the game's ones
			// NB: count+1 because the file won't save the null building, so we need to compensate
			Array<u32> oldTypeToNewType = allocateArray<u32>(arena, cBuildings->buildingTypeTable.count + 1, true);
			Array<SAVBuildingTypeEntry> buildingTypeTable = allocateArray<SAVBuildingTypeEntry>(arena, cBuildings->buildingTypeTable.count);
			if (!reader.readArray(cBuildings->buildingTypeTable, &buildingTypeTable)) break;
			for (s32 i=0; i < buildingTypeTable.count; i++)
			{
				SAVBuildingTypeEntry *entry = &buildingTypeTable[i];
				String buildingName = reader.readString(entry->name);

				BuildingDef *def = findBuildingDef(buildingName);
				if (def == null)
				{
					// The building doesn't exist in the game... we'll remap to 0
					// 
					// Ideally, we'd keep the information about what a building really is, so
					// that if it's later loaded into a game that does have that building, it'll work
					// again instead of being forever lost. But, we're talking about a situation of
					// the player messing around with the data files, or adding/removing/adding mods,
					// so IDK. The saved game is already corrupted if you load it and stuff is missing - 
					// playing at all from that point is going to break things, and if we later make
					// that stuff appear again, we're actually just breaking it a second time!
					// 
					// So maybe, the real "correct" solution is to tell the player that things are missing
					// from the game, and then just demolish the missing-id buildings.
					//
					// Mods probably want some way to define "migrations" for transforming saved games 
					// when the mod's data changes. That's probably a good idea for the base game's data
					// too, because changes happen!
					//
					// Lots to think about!
					//
					// - Sam, 11/11/2019
					//
					oldTypeToNewType[entry->typeID] = 0;
				}
				else
				{
					oldTypeToNewType[entry->typeID] = def->typeID;
				}
			}

			Array<SAVBuilding> tempBuildings = allocateArray<SAVBuilding>(arena, cBuildings->buildingCount);
			if (!reader.readBlob(cBuildings->buildings, &tempBuildings)) break;
			for (u32 buildingIndex = 0;
				buildingIndex < cBuildings->buildingCount;
				buildingIndex++)
			{
				SAVBuilding *savBuilding = &tempBuildings[buildingIndex];

				Rect2I footprint = irectXYWH(savBuilding->x, savBuilding->y, savBuilding->w, savBuilding->h);
				BuildingDef *def = getBuildingDef(oldTypeToNewType[savBuilding->typeID]);
				Building *building = addBuildingDirect(city, savBuilding->id, def, footprint, savBuilding->creationDate);
				building->variantIndex     = savBuilding->variantIndex;
				building->spriteOffset     = savBuilding->spriteOffset;
				building->currentResidents = savBuilding->currentResidents;
				building->currentJobs      = savBuilding->currentJobs;
				// Because the sprite was assigned in addBuildingDirect(), before we loaded the variant, we
				// need to overwrite the sprite here.
				// Probably there's a better way to organise this, but this works.
				// - Sam, 26/09/2020
				loadBuildingSprite(building);

				// This is a bit hacky but it's how we calculate it elsewhere
				city->zoneLayer.population[def->growsInZone] += building->currentResidents + building->currentJobs;
			}
		}
		else break;

		// ZONES
		bool readZones = reader.startSection(SAV_ZONE_ID, SAV_ZONE_VERSION);
		if (readZones)
		{
			SAVSection_Zone *cZone = reader.readStruct<SAVSection_Zone>(0);
			ZoneLayer *layer = &city->zoneLayer;

			if (!reader.readBlob(cZone->tileZone, &layer->tileZone)) break;
		}
		else break;

		// CRIME
		bool readCrime = reader.startSection(SAV_CRIME_ID, SAV_CRIME_VERSION);
		if (readCrime)
		{
			// Load Crime
			SAVSection_Crime *cCrime = reader.readStruct<SAVSection_Crime>(0);
			CrimeLayer *layer = &city->crimeLayer;

			layer->totalJailCapacity    = cCrime->totalJailCapacity;
			layer->occupiedJailCapacity = cCrime->occupiedJailCapacity;
		}
		else break;

		// EDUCATION
		bool readEducation = reader.startSection(SAV_EDUCATION_ID, SAV_EDUCATION_VERSION);
		if (readEducation)
		{
			SAVSection_Education *education = reader.readStruct<SAVSection_Education>(0);
			education = education;

			// TODO: Implement Education!
		}
		else break;

		// FIRE
		bool readFire = reader.startSection(SAV_FIRE_ID, SAV_FIRE_VERSION);
		if (readFire)
		{
			SAVSection_Fire *cFire = reader.readStruct<SAVSection_Fire>(0);
			FireLayer *layer = &city->fireLayer;

			// Active fires
			Array<SAVFire> tempFires = allocateArray<SAVFire>(arena, cFire->activeFireCount);
			if (!reader.readArray(cFire->activeFires, &tempFires)) break;
			for (u32 activeFireIndex = 0;
				activeFireIndex < cFire->activeFireCount;
				activeFireIndex++)
			{
				SAVFire *savFire = &tempFires[activeFireIndex];

				addFireRaw(city, savFire->x, savFire->y, savFire->startDate);
			}
			ASSERT((u32)layer->activeFireCount == cFire->activeFireCount);
		}
		else break;

		// HEALTH
		bool readHealth = reader.startSection(SAV_HEALTH_ID, SAV_HEALTH_VERSION);
		if (readHealth)
		{
			SAVSection_Health *cHealth = reader.readStruct<SAVSection_Health>(0);
			cHealth = cHealth;

			// TODO: Implement Health!
		}
		else break;

		// LAND VALUE
		bool readLandValue = reader.startSection(SAV_LANDVALUE_ID, SAV_LANDVALUE_VERSION);
		if (readLandValue)
		{
			SAVSection_LandValue *cLandValue = reader.readStruct<SAVSection_LandValue>(0);
			LandValueLayer *layer = &city->landValueLayer;

			if (!reader.readBlob(cLandValue->tileLandValue, &layer->tileLandValue)) break;
		}
		else break;

		// Pollution
		bool readPollution = reader.startSection(SAV_POLLUTION_ID, SAV_POLLUTION_VERSION);
		if (readPollution)
		{
			SAVSection_Pollution *cPollution = reader.readStruct<SAVSection_Pollution>(0);
			PollutionLayer *layer = &city->pollutionLayer;

			if (!reader.readBlob(cPollution->tilePollution, &layer->tilePollution)) break;
		}
		else break;

		// TRANSPORT
		bool readTransport = reader.startSection(SAV_TRANSPORT_ID, SAV_TRANSPORT_VERSION);
		if (readTransport)
		{
			SAVSection_Transport *cTransport = reader.readStruct<SAVSection_Transport>(0);
			cTransport = cTransport;

			// TODO: Implement Transport!
		}
		else break;

		// BUDGET
		bool readBudget = reader.startSection(SAV_BUDGET_ID, SAV_BUDGET_VERSION);
		if (readBudget)
		{
			SAVSection_Budget *cBudget = reader.readStruct<SAVSection_Budget>(0);
			cBudget = cBudget;

			// TODO: Implement Budget!
		}
		else break;

		// And we're done!
		succeeded = true;
		break;
	}

	return succeeded;
}
