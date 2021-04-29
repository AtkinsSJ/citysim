#pragma once

bool writeSaveFile(FileHandle *file, GameState *gameState)
{
	bool succeeded = file->isOpen;

	if (succeeded)
	{
		City *city = &gameState->city;

		BinaryFileWriter writer = startWritingFile(SAV_FILE_ID, SAV_VERSION);

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
			writer.startSection(SAV_META_ID, SAV_META_VERSION);
			WriteBufferLocation metaLoc = writer.buffer.reserveStruct<SAVSection_Meta>();

			SAVSection_Meta meta = {};
			meta.saveTimestamp = getCurrentUnixTimestamp();
			meta.cityWidth     = (u16) city->bounds.w;
			meta.cityHeight    = (u16) city->bounds.h;
			meta.funds         = city->funds;
			meta.population    = getTotalResidents(city);
			meta.jobs          = getTotalJobs(city);

			meta.cityName   = writer.appendString(city->name);
			meta.playerName = writer.appendString(city->playerName);

			// Clock
			GameClock *clock = &gameState->gameClock;
			meta.currentDate = clock->currentDay;
			meta.timeWithinDay = clock->timeWithinDay;
			
			// Camera
			Camera *camera = &renderer->worldCamera;
			meta.cameraX = camera->pos.x;
			meta.cameraY = camera->pos.y;
			meta.cameraZoom = camera->zoom;

			writer.buffer.overwriteAt(metaLoc, sizeof(meta), &meta);
			writer.endSection();
		}

		// Terrain
		{

			writer.startSection(SAV_TERRAIN_ID, SAV_TERRAIN_VERSION);
			TerrainLayer *layer = &city->terrainLayer;

			SAVSection_Terrain chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVSection_Terrain>();

			// Terrain generation parameters
			chunk.terrainGenerationSeed = layer->terrainGenerationSeed;

			// Terrain types table
			chunk.terrainTypeCount = terrainCatalogue.terrainDefs.count - 1; // Not the null def!
			chunk.offsetForTerrainTypeTable = writer.getSectionRelativeOffset();
			for (auto it = terrainCatalogue.terrainDefs.iterate(); it.hasNext(); it.next())
			{
				TerrainDef *def = it.get();
				if (def->typeID == 0) continue; // Skip the null terrain def!

				u32 typeID = def->typeID;
				u32 nameLength = def->name.length;

				// 4 byte int id, 4 byte length, then the text as bytes
				writer.buffer.appendLiteral(typeID);
				writer.buffer.appendLiteral(nameLength);
				writer.buffer.appendBytes(nameLength, def->name.chars);
			}

			// Tile terrain type (u8)
			chunk.tileTerrainType = writer.appendBlob(&layer->tileTerrainType, Blob_RLE_S8);

			// Tile height (u8)
			chunk.tileHeight = writer.appendBlob(&layer->tileHeight, Blob_RLE_S8);

			// Tile sprite offset (u8)
			chunk.tileSpriteOffset = writer.appendBlob(&layer->tileSpriteOffset, Blob_Uncompressed);

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Buildings
		{
			writer.startSection(SAV_BUILDING_ID, SAV_BUILDING_VERSION);

			SAVSection_Buildings chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVSection_Buildings>();

			// Building types table
			chunk.buildingTypeCount = buildingCatalogue.allBuildings.count - 1; // Not the null def!
			chunk.offsetForBuildingTypeTable = writer.getSectionRelativeOffset();
			for (auto it = buildingCatalogue.allBuildings.iterate(); it.hasNext(); it.next())
			{
				BuildingDef *def = it.get();
				if (def->typeID == 0) continue; // Skip the null building def!

				u32 typeID = def->typeID;
				u32 nameLength = def->name.length;

				// 4 byte int id, 4 byte length, then the text as bytes
				writer.buffer.appendLiteral(typeID);
				writer.buffer.appendLiteral(nameLength);
				writer.buffer.appendBytes(nameLength, def->name.chars);
			}

			// Highest ID
			chunk.highestBuildingID = city->highestBuildingID;

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
			chunk.buildingCount = city->buildings.count - 1; // Not the null building!

			SAVBuilding *tempBuildings = allocateMultiple<SAVBuilding>(tempArena, chunk.buildingCount);
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
			chunk.buildings = writer.appendBlob(chunk.buildingCount * sizeof(SAVBuilding), (u8*)tempBuildings, Blob_Uncompressed);

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Zones
		{
			writer.startSection(SAV_ZONE_ID, SAV_ZONE_VERSION);
			ZoneLayer *layer = &city->zoneLayer;

			SAVSection_Zone chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVSection_Zone>();

			// Tile zones
			chunk.tileZone = writer.appendBlob(&layer->tileZone, Blob_RLE_S8);

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Budget
		{
			writer.startSection(SAV_BUDGET_ID, SAV_BUDGET_VERSION);
			// BudgetLayer *layer = &city->budgetLayer;

			SAVSection_Budget chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVSection_Budget>();

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Crime
		{
			writer.startSection(SAV_CRIME_ID, SAV_CRIME_VERSION);
			CrimeLayer *layer = &city->crimeLayer;

			SAVSection_Crime chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVSection_Crime>();

			chunk.totalJailCapacity = layer->totalJailCapacity;
			chunk.occupiedJailCapacity = layer->occupiedJailCapacity;

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Education
		{
			writer.startSection(SAV_EDUCATION_ID, SAV_EDUCATION_VERSION);
			// EducationLayer *layer = &city->educationLayer;

			SAVSection_Education chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVSection_Education>();

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Fire
		{
			writer.startSection(SAV_FIRE_ID, SAV_FIRE_VERSION);
			FireLayer *layer = &city->fireLayer;

			SAVSection_Fire chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVSection_Fire>();

			// Active fires
			chunk.activeFireCount = layer->activeFireCount;
			Array<SAVFire> tempFires = allocateArray<SAVFire>(tempArena, chunk.activeFireCount);
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
			chunk.activeFires = writer.appendArray<SAVFire>(tempFires);

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Health
		{
			writer.startSection(SAV_HEALTH_ID, SAV_HEALTH_VERSION);
			// HealthLayer *layer = &city->healthLayer;

			SAVSection_Health chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVSection_Health>();

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Land Value
		{
			writer.startSection(SAV_LANDVALUE_ID, SAV_LANDVALUE_VERSION);
			LandValueLayer *layer = &city->landValueLayer;

			SAVSection_LandValue chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVSection_LandValue>();

			// Tile land value
			chunk.tileLandValue = writer.appendBlob(&layer->tileLandValue, Blob_Uncompressed);

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Pollution
		{
			writer.startSection(SAV_POLLUTION_ID, SAV_POLLUTION_VERSION);
			PollutionLayer *layer = &city->pollutionLayer;

			SAVSection_Pollution chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVSection_Pollution>();

			// Tile pollution
			chunk.tilePollution = writer.appendBlob(&layer->tilePollution, Blob_Uncompressed);

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Transport
		{
			writer.startSection(SAV_TRANSPORT_ID, SAV_TRANSPORT_VERSION);
			// TransportLayer *layer = &city->transportLayer;

			SAVSection_Transport chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVSection_Transport>();

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}


		// succeeded = writeToFile(file, &writer.buffer);

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

	BinaryFileReader reader = readBinaryFile(file, SAV_FILE_ID, tempArena);
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
			SAVSection_Terrain *cTerrain = reader.readStruct<SAVSection_Terrain>(0);
			TerrainLayer *layer = &city->terrainLayer;

			layer->terrainGenerationSeed = cTerrain->terrainGenerationSeed;

			// Map the file's terrain type IDs to the game's ones
			// NB: count+1 because the file won't save the null terrain, so we need to compensate
			Array<u8> oldTypeToNewType = allocateArray<u8>(tempArena, cTerrain->terrainTypeCount + 1, true);
			// TODO: Stop using internal method!
			u8 *at = reader.sectionMemoryAt(cTerrain->offsetForTerrainTypeTable);
			for (u32 i = 0; i < cTerrain->terrainTypeCount; i++)
			{
				u32 terrainType = *(u32 *)at;
				at += sizeof(u32);

				u32 nameLength = *(u32 *)at;
				at += sizeof(u32);

				String terrainName = makeString((char*)at, nameLength, true);
				at += nameLength;

				oldTypeToNewType[terrainType] = findTerrainTypeByName(terrainName);
			}

			// Terrain type
			if (!reader.readBlob(cTerrain->tileTerrainType, &layer->tileTerrainType)) break;
			for (s32 y = 0; y < city->bounds.y; y++)
			{
				for (s32 x = 0; x < city->bounds.x; x++)
				{
					layer->tileTerrainType.set(x, y,  oldTypeToNewType[layer->tileTerrainType.get(x, y)]);
				}
			}

			// Terrain height
			if (!reader.readBlob(cTerrain->tileHeight, &layer->tileHeight)) break;

			// Sprite offset
			if (!reader.readBlob(cTerrain->tileSpriteOffset, &layer->tileSpriteOffset)) break;

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
			Array<s32> oldTypeToNewType = allocateArray<s32>(tempArena, cBuildings->buildingTypeCount+1, true);
			// TODO: Stop using internal method!
			u8 *at = reader.sectionMemoryAt(cBuildings->offsetForBuildingTypeTable);
			for (u32 i = 0; i < cBuildings->buildingTypeCount; i++)
			{
				u32 buildingType = *(u32 *)at;
				at += sizeof(u32);

				u32 nameLength = *(u32 *)at;
				at += sizeof(u32);

				String buildingName = makeString((char*)at, nameLength, true);
				at += nameLength;

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
					oldTypeToNewType[buildingType] = 0;
				}
				else
				{
					oldTypeToNewType[buildingType] = def->typeID;
				}
			}

			Array<SAVBuilding> tempBuildings = allocateArray<SAVBuilding>(tempArena, cBuildings->buildingCount);
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
			Array<SAVFire> tempFires = allocateArray<SAVFire>(tempArena, cFire->activeFireCount);
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
