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
		writer.addTOCEntry(SAV_BDGT_ID);
		writer.addTOCEntry(SAV_BLDG_ID);
		writer.addTOCEntry(SAV_CRIM_ID);
		writer.addTOCEntry(SAV_EDUC_ID);
		writer.addTOCEntry(SAV_FIRE_ID);
		writer.addTOCEntry(SAV_HLTH_ID);
		writer.addTOCEntry(SAV_LVAL_ID);
		writer.addTOCEntry(SAV_PLTN_ID);
		writer.addTOCEntry(SAV_TERR_ID);
		writer.addTOCEntry(SAV_TPRT_ID);
		writer.addTOCEntry(SAV_ZONE_ID);

		// Meta
		{
			writer.startSection(SAV_META_ID, SAV_META_VERSION);
			WriteBufferLocation metaLoc = writer.buffer.reserveStruct<SAVChunk_Meta>();

			SAVChunk_Meta meta = {};
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

			writer.startSection(SAV_TERR_ID, SAV_TERR_VERSION);
			TerrainLayer *layer = &city->terrainLayer;

			SAVChunk_Terrain chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVChunk_Terrain>();

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
			writer.startSection(SAV_BLDG_ID, SAV_BLDG_VERSION);

			SAVChunk_Buildings chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVChunk_Buildings>();

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

			SAVChunk_Zone chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVChunk_Zone>();

			// Tile zones
			chunk.tileZone = writer.appendBlob(&layer->tileZone, Blob_RLE_S8);

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Budget
		{
			writer.startSection(SAV_BDGT_ID, SAV_BDGT_VERSION);
			// BudgetLayer *layer = &city->budgetLayer;

			SAVChunk_Budget chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVChunk_Budget>();

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Crime
		{
			writer.startSection(SAV_CRIM_ID, SAV_CRIM_VERSION);
			CrimeLayer *layer = &city->crimeLayer;

			SAVChunk_Crime chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVChunk_Crime>();

			chunk.totalJailCapacity = layer->totalJailCapacity;
			chunk.occupiedJailCapacity = layer->occupiedJailCapacity;

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Education
		{
			writer.startSection(SAV_EDUC_ID, SAV_EDUC_VERSION);
			// EducationLayer *layer = &city->educationLayer;

			SAVChunk_Education chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVChunk_Education>();

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Fire
		{
			writer.startSection(SAV_FIRE_ID, SAV_FIRE_VERSION);
			FireLayer *layer = &city->fireLayer;

			SAVChunk_Fire chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVChunk_Fire>();

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
			writer.startSection(SAV_HLTH_ID, SAV_HLTH_VERSION);
			// HealthLayer *layer = &city->healthLayer;

			SAVChunk_Health chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVChunk_Health>();

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Land Value
		{
			writer.startSection(SAV_LVAL_ID, SAV_LVAL_VERSION);
			LandValueLayer *layer = &city->landValueLayer;

			SAVChunk_LandValue chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVChunk_LandValue>();

			// Tile land value
			chunk.tileLandValue = writer.appendBlob(&layer->tileLandValue, Blob_Uncompressed);

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Pollution
		{
			writer.startSection(SAV_PLTN_ID, SAV_PLTN_VERSION);
			PollutionLayer *layer = &city->pollutionLayer;

			SAVChunk_Pollution chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVChunk_Pollution>();

			// Tile pollution
			chunk.tilePollution = writer.appendBlob(&layer->tilePollution, Blob_Uncompressed);

			writer.buffer.overwriteAt(startOfChunk, sizeof(chunk), &chunk);

			writer.endSection();
		}

		// Transport
		{
			writer.startSection(SAV_TPRT_ID, SAV_TPRT_VERSION);
			// TransportLayer *layer = &city->transportLayer;

			SAVChunk_Transport chunk = {};
			WriteBufferLocation startOfChunk = writer.buffer.reserveStruct<SAVChunk_Transport>();

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

	BinaryFileReader reader = readBinaryFile(file, SAV_FILE_ID);
	if (reader.isValidFile)
	{
		s32 cityTileCount = 0;

		// META
		bool readMeta = reader.startSection(SAV_META_ID, SAV_META_VERSION);
		if (readMeta)
		{
			SAVChunk_Meta *meta = reader.readStruct<SAVChunk_Meta>(0);

			String cityName = reader.readString(meta->cityName);
			String playerName = reader.readString(meta->playerName);
			initCity(&gameState->gameArena, city, meta->cityWidth, meta->cityHeight, cityName, playerName, meta->funds);
			cityTileCount = city->bounds.w * city->bounds.h;

			// Clock
			initGameClock(&gameState->gameClock, meta->currentDate, meta->timeWithinDay);

			// Camera
			setCameraPos(&renderer->worldCamera, v2(meta->cameraX, meta->cameraY), meta->cameraZoom);
		}

		// TERRAIN
		bool readTerrain = reader.startSection(SAV_TERR_ID, SAV_TERR_VERSION);
		if (readTerrain)
		{
			SAVChunk_Terrain *cTerrain = reader.readStruct<SAVChunk_Terrain>(0);
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
			if (reader.readBlob(cTerrain->tileTerrainType, &layer->tileTerrainType))
			{
				for (s32 y = 0; y < city->bounds.y; y++)
				{
					for (s32 x = 0; x < city->bounds.x; x++)
					{
						layer->tileTerrainType.set(x, y,  oldTypeToNewType[layer->tileTerrainType.get(x, y)]);
					}
				}
			}

			// Terrain height
			reader.readBlob(cTerrain->tileHeight, &layer->tileHeight);

			// Sprite offset
			reader.readBlob(cTerrain->tileSpriteOffset, &layer->tileSpriteOffset);

			assignTerrainSprites(city, city->bounds);
		}

		succeeded = true;
	}

//
//
//
//
//
//
//
//
#if 0
	File saveFile = readFile(file);
	bool succeeded = saveFile.isLoaded;

	// NB: We use a while() so that we can break out of it easily below.
	// I guess I could use gotos instead, but that feels worse than abusing a while, somehow.
	while (succeeded)
	{
		u8 *start = saveFile.data.memory;
		u8 *eof = start + saveFile.data.size;

		u8 *pos = start;

		// File Header
		FileHeader *fileHeader = (FileHeader *) pos;
		pos += sizeof(FileHeader);
		if (pos > eof)
		{
			succeeded = false;
			break;
		}

		if (!fileHeaderIsValid(fileHeader, saveFile.name, SAV_FILE_ID))
		{
			succeeded = false;
			break;
		}

		// Macros are horrible, but I don't think it's possible to do these another way!
		#define READ_CHUNK(Type) (Type *) pos; pos += header->length; if (pos > eof) { succeeded = false; break; }
		#define CHECK_VERSION(Chunk) if (header->version > SAV_ ## Chunk ## _VERSION) \
				{\
					logError("{0} chunk in save file '{1}' uses a newer save file format than we understand. {0} version is '{2}', maximum we support is '{3}'"_s, {\
						#Chunk ## _s, saveFile.name, formatInt(header->version), formatInt(SAV_ ## Chunk ## _VERSION),\
					});\
					succeeded = false;\
					break;\
				}
		#define REQUIRE_META(Chunk) if (!hasLoadedMeta) { logError("{0} chunk found before META chunk. META must be the first chunk in the file!"_s, {#Chunk ## _s}); succeeded = false; break;}

		// Loop over file chunks
		// NB: META chunk must be before any other data chunks, because without it we can't sensibly read
		// any of the tile data!
		bool hasLoadedMeta = false;
		s32 cityTileCount = 0;
		while (pos < eof)
		{
			FileSectionHeader *header = (FileSectionHeader *) pos;
			pos += sizeof(FileSectionHeader);
			if (pos > eof)
			{
				succeeded = false;
				break;
			}

			// NB: The SAV_XXX_ID stuff isn't constant at compile time, or something,
			// so we can't switch() on it, even though it looks like it should work!
			if (header->identifier == SAV_META_ID)
			{
				// Load Meta
				CHECK_VERSION(META);

				u8 *startOfChunk = pos;
				SAVChunk_Meta *cMeta = READ_CHUNK(SAVChunk_Meta);

				String cityName = readString(cMeta->cityName, startOfChunk);
				String playerName = readString(cMeta->playerName, startOfChunk);
				initCity(&gameState->gameArena, city, cMeta->cityWidth, cMeta->cityHeight, cityName, playerName, cMeta->funds);
				cityTileCount = city->bounds.w * city->bounds.h;

				// Clock
				initGameClock(&gameState->gameClock, cMeta->currentDate, cMeta->timeWithinDay);

				// Camera
				setCameraPos(&renderer->worldCamera, v2(cMeta->cameraX, cMeta->cameraY), cMeta->cameraZoom);

				hasLoadedMeta = true;
			}
			else if (header->identifier == SAV_BDGT_ID)
			{
				// Load Budget
				CHECK_VERSION(BDGT);
				REQUIRE_META(BDGT);

				// u8 *startOfChunk = pos;
				SAVChunk_Budget *budget = READ_CHUNK(SAVChunk_Budget);
				budget = budget;

				// TODO: Implement!
			}
			else if (header->identifier == SAV_BLDG_ID)
			{
				// Load Building
				CHECK_VERSION(BLDG);
				REQUIRE_META(BLDG);

				u8 *startOfChunk = pos;
				SAVChunk_Buildings *cBuildings = READ_CHUNK(SAVChunk_Buildings);

				city->highestBuildingID = cBuildings->highestBuildingID;

				// Map the file's building type IDs to the game's ones
				// NB: count+1 because the file won't save the null building, so we need to compensate
				Array<s32> oldTypeToNewType = allocateArray<s32>(tempArena, cBuildings->buildingTypeCount+1, true);
				u8 *at = startOfChunk + cBuildings->offsetForBuildingTypeTable;
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

				SAVBuilding *tempBuildings = allocateMultiple<SAVBuilding>(tempArena, cBuildings->buildingCount);
				if (decodeBlob(cBuildings->buildings, startOfChunk, (u8*)tempBuildings, cBuildings->buildingCount * sizeof(SAVBuilding)))
				{
					for (u32 buildingIndex = 0;
						buildingIndex < cBuildings->buildingCount;
						buildingIndex++)
					{
						SAVBuilding *savBuilding = tempBuildings + buildingIndex;

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
			}
			else if (header->identifier == SAV_CRIM_ID)
			{
				// Load Crime
				CHECK_VERSION(CRIM);
				REQUIRE_META(CRIM);

				// u8 *startOfChunk = pos;
				SAVChunk_Crime *cCrime = READ_CHUNK(SAVChunk_Crime);
				CrimeLayer *layer = &city->crimeLayer;

				layer->totalJailCapacity    = cCrime->totalJailCapacity;
				layer->occupiedJailCapacity = cCrime->occupiedJailCapacity;
			}
			else if (header->identifier == SAV_EDUC_ID)
			{
				// Load Education
				CHECK_VERSION(EDUC);
				REQUIRE_META(EDUC);

				// u8 *startOfChunk = pos;
				SAVChunk_Education *education = READ_CHUNK(SAVChunk_Education);
				education = education;

				// TODO: Implement Education!
			}
			else if (header->identifier == SAV_FIRE_ID)
			{
				// Load Fire
				CHECK_VERSION(FIRE);
				REQUIRE_META(FIRE);

				u8 *startOfChunk = pos;
				SAVChunk_Fire *cFire = READ_CHUNK(SAVChunk_Fire);
				FireLayer *layer = &city->fireLayer;

				// Active fires
				Array<SAVFire> tempFires = allocateArray<SAVFire>(tempArena, cFire->activeFireCount);
				if (readArray(cFire->activeFires, startOfChunk, &tempFires))
				{
					for (u32 activeFireIndex = 0;
						activeFireIndex < cFire->activeFireCount;
						activeFireIndex++)
					{
						SAVFire *savFire = &tempFires[activeFireIndex];

						addFireRaw(city, savFire->x, savFire->y, savFire->startDate);
					}
					ASSERT((u32)layer->activeFireCount == cFire->activeFireCount);
				}
			}
			else if (header->identifier == SAV_HLTH_ID)
			{
				// Load Health
				CHECK_VERSION(HLTH);
				REQUIRE_META(HLTH);

				// u8 *startOfChunk = pos;
				SAVChunk_Health *cHealth = READ_CHUNK(SAVChunk_Health);
				cHealth = cHealth;

				// TODO: Implement Health!
			}
			else if (header->identifier == SAV_LVAL_ID)
			{
				// Load Land Value
				CHECK_VERSION(LVAL);
				REQUIRE_META(LVAL);

				u8 *startOfChunk = pos;
				SAVChunk_LandValue *cLandValue = READ_CHUNK(SAVChunk_LandValue);
				LandValueLayer *layer = &city->landValueLayer;

				decodeBlob(cLandValue->tileLandValue, startOfChunk, &layer->tileLandValue);
			}
			else if (header->identifier == SAV_PLTN_ID)
			{
				// Load Pollution
				CHECK_VERSION(PLTN);
				REQUIRE_META(PLTN);

				u8 *startOfChunk = pos;
				SAVChunk_Pollution *cPollution = READ_CHUNK(SAVChunk_Pollution);
				PollutionLayer *layer = &city->pollutionLayer;

				decodeBlob(cPollution->tilePollution, startOfChunk, &layer->tilePollution);
			}
			else if (header->identifier == SAV_TERR_ID)
			{
				// Load Terrain
				CHECK_VERSION(TERR);
				REQUIRE_META(TERR);

				u8 *startOfChunk = pos;
				SAVChunk_Terrain *cTerrain = READ_CHUNK(SAVChunk_Terrain);
				TerrainLayer *layer = &city->terrainLayer;

				layer->terrainGenerationSeed = cTerrain->terrainGenerationSeed;

				// Map the file's terrain type IDs to the game's ones
				// NB: count+1 because the file won't save the null terrain, so we need to compensate
				Array<u8> oldTypeToNewType = allocateArray<u8>(tempArena, cTerrain->terrainTypeCount + 1, true);
				u8 *at = startOfChunk + cTerrain->offsetForTerrainTypeTable;
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
				u8 *decodedTileTerrainType = allocateMultiple<u8>(tempArena, cityTileCount);
				if (decodeBlob(cTerrain->tileTerrainType, startOfChunk, decodedTileTerrainType, cityTileCount))
				{
					for (s32 i = 0; i < cityTileCount; i++)
					{
						decodedTileTerrainType[i] = oldTypeToNewType[ decodedTileTerrainType[i] ];
					}
					copyMemory(decodedTileTerrainType, layer->tileTerrainType.items, cityTileCount);
				}

				// Terrain height
				decodeBlob(cTerrain->tileHeight, startOfChunk, &layer->tileHeight);

				// Sprite offset
				decodeBlob(cTerrain->tileSpriteOffset, startOfChunk, &layer->tileSpriteOffset);

				assignTerrainSprites(city, city->bounds);
			}
			else if (header->identifier == SAV_TPRT_ID)
			{
				// Load Transport
				CHECK_VERSION(TPRT);
				REQUIRE_META(TPRT);

				// u8 *startOfChunk = pos;
				SAVChunk_Transport *cTransport = READ_CHUNK(SAVChunk_Transport);
				cTransport = cTransport;

				// TODO: Implement Transport!
			}
			else if (header->identifier == SAV_ZONE_ID)
			{
				// Load Zone
				CHECK_VERSION(ZONE);
				REQUIRE_META(ZONE);

				u8 *startOfChunk = pos;
				SAVChunk_Zone *cZone = READ_CHUNK(SAVChunk_Zone);
				ZoneLayer *layer = &city->zoneLayer;

				decodeBlob(cZone->tileZone, startOfChunk, &layer->tileZone);
			}
		}

		#undef READ

		// This break is because we succeeded!
		succeeded = true;
		break;
	}
#endif

	return succeeded;
}
