#pragma once

bool writeSaveFile(FileHandle *file, City *city)
{
	struct ChunkHeaderWrapper
	{
		WriteBuffer *buffer;
		s32 startOfChunkHeader;
		s32 startOfChunkData;

		SAVChunkHeader chunkHeader;

		ChunkHeaderWrapper(WriteBuffer *buffer, const u8 chunkID[4], u8 chunkVersion)
		{
			this->buffer = buffer;
			this->startOfChunkHeader = reserve(buffer, sizeof(SAVChunkHeader));
			this->startOfChunkData = getCurrentPosition(buffer);

			this->chunkHeader = {};
			copyMemory<u8>(chunkID, this->chunkHeader.identifier, 4);
			this->chunkHeader.version = chunkVersion;
		}

		~ChunkHeaderWrapper()
		{
			this->chunkHeader.length = getCurrentPosition(buffer) - this->startOfChunkData;
			overwriteAt(buffer, startOfChunkHeader, sizeof(SAVChunkHeader), &this->chunkHeader);
		}
	};


	bool succeeded = file->isOpen;

	WriteBuffer buffer;

	if (succeeded)
	{
		initWriteBuffer(&buffer);

		// File Header
		SAVFileHeader fileHeader = SAVFileHeader();
		append(&buffer, sizeof(fileHeader), &fileHeader);

		// Meta
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_META_ID, SAV_META_VERSION);

			SAVChunk_Meta meta = {};
			meta.saveTimestamp = getCurrentUnixTimestamp();
			meta.cityWidth     = (u16) city->bounds.w;
			meta.cityHeight    = (u16) city->bounds.h;
			meta.funds         = city->funds;
			meta.population    = getTotalResidents(city);
			meta.jobs          = getTotalJobs(city);

			s32 stringOffset = sizeof(meta);
			meta.cityName = {(u32)city->name.length, (u32)stringOffset};
			stringOffset += meta.cityName.length;
			meta.playerName = {(u32)city->playerName.length, (u32)stringOffset};
			stringOffset += meta.playerName.length;

			// Write the data
			appendStruct(&buffer, &meta);
			append(&buffer, city->name.length, city->name.chars);
			append(&buffer, city->playerName.length, city->playerName.chars);
		}

		// Terrain
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_TERR_ID, SAV_TERR_VERSION);
			TerrainLayer *layer = &city->terrainLayer;

			SAVChunk_Terrain chunk = {};
			s32 startOfChunk = reserve(&buffer, sizeof(chunk));
			s32 offset = sizeof(chunk);

			// Terrain generation parameters
			chunk.terrainGenerationSeed = layer->terrainGenerationSeed;

			// Terrain types table
			chunk.terrainTypeCount = terrainCatalogue.terrainDefs.count - 1; // Not the null def!
			chunk.offsetForTerrainTypeTable = offset;
			for (auto it = iterate(&terrainCatalogue.terrainDefs); hasNext(&it); next(&it))
			{
				TerrainDef *def = get(&it);
				if (def->typeID == 0) continue; // Skip the null terrain def!

				u32 typeID = def->typeID;
				u32 nameLength = def->name.length;

				// 4 byte int id, 4 byte length, then the text as bytes
				appendStruct(&buffer, &typeID);
				appendStruct(&buffer, &nameLength);
				append(&buffer, nameLength, def->name.chars);

				offset += sizeof(typeID) + sizeof(nameLength) + nameLength;
			}

			// Tile terrain type (u8)
			chunk.tileTerrainType = appendBlob(offset, &buffer, &layer->tileTerrainType, Blob_RLE_S8);
			offset += chunk.tileTerrainType.length;

			// Tile height (u8)
			chunk.tileHeight = appendBlob(offset, &buffer, &layer->tileHeight, Blob_RLE_S8);
			offset += chunk.tileHeight.length;

			// Tile sprite offset (u8)
			chunk.tileSpriteOffset = appendBlob(offset, &buffer, &layer->tileSpriteOffset, Blob_Uncompressed);
			offset += chunk.tileSpriteOffset.length;

			overwriteAt(&buffer, startOfChunk, sizeof(chunk), &chunk);
		}

		// Buildings
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_BLDG_ID, SAV_BLDG_VERSION);

			SAVChunk_Buildings chunk = {};
			s32 startOfChunk = reserve(&buffer, sizeof(chunk));
			s32 offset = sizeof(chunk);

			// Building types table
			chunk.buildingTypeCount = buildingCatalogue.allBuildings.count - 1; // Not the null def!
			chunk.offsetForBuildingTypeTable = offset;
			for (auto it = iterate(&buildingCatalogue.allBuildings); hasNext(&it); next(&it))
			{
				BuildingDef *def = get(&it);
				if (def->typeID == 0) continue; // Skip the null building def!

				u32 typeID = def->typeID;
				u32 nameLength = def->name.length;

				// 4 byte int id, 4 byte length, then the text as bytes
				appendStruct(&buffer, &typeID);
				appendStruct(&buffer, &nameLength);
				append(&buffer, nameLength, def->name.chars);

				offset += sizeof(typeID) + sizeof(nameLength) + nameLength;
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

			for (auto it = iterate(&city->buildings); hasNext(&it); next(&it))
			{
				Building *building = get(&it);
				if (building->id == 0) continue; // Skip the null building!

				SAVBuilding *sb = tempBuildings + tempBuildingIndex;
				*sb = {};
				sb->id = building->id;
				sb->typeID = building->typeID;
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
			chunk.buildings = appendBlob(offset, &buffer, chunk.buildingCount * sizeof(SAVBuilding), (u8*)tempBuildings, Blob_Uncompressed);
			offset += chunk.buildings.length;

			overwriteAt(&buffer, startOfChunk, sizeof(chunk), &chunk);
		}

		// Zones
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_ZONE_ID, SAV_ZONE_VERSION);
			ZoneLayer *layer = &city->zoneLayer;

			SAVChunk_Zone chunk = {};
			s32 startOfChunk = reserve(&buffer, sizeof(chunk));
			s32 offset = sizeof(chunk);

			// Tile zones
			chunk.tileZone = appendBlob(offset, &buffer, &layer->tileZone, Blob_RLE_S8);
			offset += chunk.tileZone.length;

			overwriteAt(&buffer, startOfChunk, sizeof(chunk), &chunk);
		}

		// Budget
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_BDGT_ID, SAV_BDGT_VERSION);
			// BudgetLayer *layer = &city->budgetLayer;

			SAVChunk_Budget chunk = {};
			s32 startOfChunk = reserve(&buffer, sizeof(chunk));
			// s32 offset = sizeof(chunk);

			overwriteAt(&buffer, startOfChunk, sizeof(chunk), &chunk);
		}

		// Crime
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_CRIM_ID, SAV_CRIM_VERSION);
			CrimeLayer *layer = &city->crimeLayer;

			SAVChunk_Crime chunk = {};
			s32 startOfChunk = reserve(&buffer, sizeof(chunk));
			// s32 offset = sizeof(chunk);

			chunk.totalJailCapacity = layer->totalJailCapacity;
			chunk.occupiedJailCapacity = layer->occupiedJailCapacity;

			overwriteAt(&buffer, startOfChunk, sizeof(chunk), &chunk);
		}

		// Education
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_EDUC_ID, SAV_EDUC_VERSION);
			// EducationLayer *layer = &city->educationLayer;

			SAVChunk_Education chunk = {};
			s32 startOfChunk = reserve(&buffer, sizeof(chunk));
			// s32 offset = sizeof(chunk);

			overwriteAt(&buffer, startOfChunk, sizeof(chunk), &chunk);
		}

		// Fire
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_FIRE_ID, SAV_FIRE_VERSION);
			FireLayer *layer = &city->fireLayer;

			SAVChunk_Fire chunk = {};
			s32 startOfChunk = reserve(&buffer, sizeof(chunk));
			s32 offset = sizeof(chunk);

			// Active fires
			chunk.activeFireCount = layer->activeFireCount;
			SAVFire *tempFires = allocateMultiple<SAVFire>(tempArena, chunk.activeFireCount);
			s32 tempFireIndex = 0;
			// ughhhh I have to iterate the sectors to get this information!
			for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&layer->sectors); sectorIndex++)
			{
				FireSector *sector = getSectorByIndex(&layer->sectors, sectorIndex);

				for (auto it = iterate(&sector->activeFires); hasNext(&it); next(&it))
				{
					Fire *fire = get(&it);
					SAVFire *savFire = tempFires + tempFireIndex++;
					*savFire = {};
					savFire->x = (u16) fire->pos.x;
					savFire->y = (u16) fire->pos.y;
				}
			}
			chunk.activeFires = appendBlob(offset, &buffer, chunk.activeFireCount * sizeof(SAVFire), (u8*)tempFires, Blob_Uncompressed);
			offset += chunk.activeFires.length;

			overwriteAt(&buffer, startOfChunk, sizeof(chunk), &chunk);
		}

		// Health
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_HLTH_ID, SAV_HLTH_VERSION);
			// HealthLayer *layer = &city->healthLayer;

			SAVChunk_Health chunk = {};
			s32 startOfChunk = reserve(&buffer, sizeof(chunk));
			// s32 offset = sizeof(chunk);

			overwriteAt(&buffer, startOfChunk, sizeof(chunk), &chunk);
		}

		// Land Value
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_LVAL_ID, SAV_LVAL_VERSION);
			LandValueLayer *layer = &city->landValueLayer;

			SAVChunk_LandValue chunk = {};
			s32 startOfChunk = reserve(&buffer, sizeof(chunk));
			s32 offset = sizeof(chunk);

			// Tile land value
			chunk.tileLandValue = appendBlob(offset, &buffer, &layer->tileLandValue, Blob_Uncompressed);
			offset += chunk.tileLandValue.length;

			overwriteAt(&buffer, startOfChunk, sizeof(chunk), &chunk);
		}

		// Pollution
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_PLTN_ID, SAV_PLTN_VERSION);
			PollutionLayer *layer = &city->pollutionLayer;

			SAVChunk_Pollution chunk = {};
			s32 startOfChunk = reserve(&buffer, sizeof(chunk));
			s32 offset = sizeof(chunk);

			// Tile pollution
			chunk.tilePollution = appendBlob(offset, &buffer, &layer->tilePollution, Blob_Uncompressed);
			offset += chunk.tilePollution.length;

			overwriteAt(&buffer, startOfChunk, sizeof(chunk), &chunk);
		}

		// Transport
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_TPRT_ID, SAV_TPRT_VERSION);
			// TransportLayer *layer = &city->transportLayer;

			SAVChunk_Transport chunk = {};
			s32 startOfChunk = reserve(&buffer, sizeof(chunk));
			// s32 offset = sizeof(chunk);

			overwriteAt(&buffer, startOfChunk, sizeof(chunk), &chunk);
		}


		succeeded = writeToFile(file, &buffer);
	}

	return succeeded;
}

bool loadSaveFile(FileHandle *file, City *city, MemoryArena *gameArena)
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
		SAVFileHeader *fileHeader = (SAVFileHeader *) pos;
		pos += sizeof(SAVFileHeader);
		if (pos > eof)
		{
			succeeded = false;
			break;
		}

		if (!fileHeaderIsValid(fileHeader, saveFile.name))
		{
			succeeded = false;
			break;
		}

		if (!checkFileHeaderVersion(fileHeader, saveFile.name))
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
			SAVChunkHeader *header = (SAVChunkHeader *) pos;
			pos += sizeof(SAVChunkHeader);
			if (pos > eof)
			{
				succeeded = false;
				break;
			}

			// I considered doing a hash table here, but it's really overkill - we only see one of
			// each of these, and there aren't that many of them, so I doubt it'll make any notable
			// speed difference, but it WOULD make a big complexity difference!
			// - Sam, 16/10/2019
			//
			// ... Actually, the identifiers are all 4 bytes, so I could just case to a u32 and then
			// do a simple == or even a switch!
			// @Speed
			// - Sam, 10/11/2019

			if (identifiersAreEqual(header->identifier, SAV_META_ID))
			{
				// Load Meta
				CHECK_VERSION(META);

				u8 *startOfChunk = pos;
				SAVChunk_Meta *cMeta = READ_CHUNK(SAVChunk_Meta);

				String cityName = loadString(cMeta->cityName, startOfChunk, gameArena);
				String playerName = loadString(cMeta->playerName, startOfChunk, gameArena);
				initCity(gameArena, city, cMeta->cityWidth, cMeta->cityHeight, cityName, playerName, cMeta->funds);
				cityTileCount = city->bounds.w * city->bounds.h;

				hasLoadedMeta = true;
			}
			else if (identifiersAreEqual(header->identifier, SAV_BDGT_ID))
			{
				// Load Budget
				CHECK_VERSION(BDGT);
				REQUIRE_META(BDGT);

				// u8 *startOfChunk = pos;
				SAVChunk_Budget *budget = READ_CHUNK(SAVChunk_Budget);
				budget = budget;

				// TODO: Implement!
			}
			else if (identifiersAreEqual(header->identifier, SAV_BLDG_ID))
			{
				// Load Building
				CHECK_VERSION(BLDG);
				REQUIRE_META(BLDG);

				u8 *startOfChunk = pos;
				SAVChunk_Buildings *cBuildings = READ_CHUNK(SAVChunk_Buildings);

				city->highestBuildingID = cBuildings->highestBuildingID;

				// Map the file's building type IDs to the game's ones
				// NB: count+1 because the file won't save the null building, so we need to compensate
				Array<s32> oldTypeToNewType = allocateArray<s32>(tempArena, cBuildings->buildingTypeCount+1);
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
						Building *building = addBuildingDirect(city, savBuilding->id, def, footprint);
						building->variantIndex     = savBuilding->variantIndex;
						building->spriteOffset     = savBuilding->spriteOffset;
						building->currentResidents = savBuilding->currentResidents;
						building->currentJobs      = savBuilding->currentJobs;

						// This is a bit hacky but it's how we calculate it elsewhere
						city->zoneLayer.population[def->growsInZone] += building->currentResidents + building->currentJobs;
					}
				}
			}
			else if (identifiersAreEqual(header->identifier, SAV_CRIM_ID))
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
			else if (identifiersAreEqual(header->identifier, SAV_EDUC_ID))
			{
				// Load Education
				CHECK_VERSION(EDUC);
				REQUIRE_META(EDUC);

				// u8 *startOfChunk = pos;
				SAVChunk_Education *education = READ_CHUNK(SAVChunk_Education);
				education = education;

				// TODO: Implement Education!
			}
			else if (identifiersAreEqual(header->identifier, SAV_FIRE_ID))
			{
				// Load Fire
				CHECK_VERSION(FIRE);
				REQUIRE_META(FIRE);

				u8 *startOfChunk = pos;
				SAVChunk_Fire *cFire = READ_CHUNK(SAVChunk_Fire);
				FireLayer *layer = &city->fireLayer;

				// Active fires
				SAVFire *tempFires = allocateMultiple<SAVFire>(tempArena, cFire->activeFireCount);
				if (decodeBlob(cFire->activeFires, startOfChunk, (u8*)tempFires, cFire->activeFireCount * sizeof(SAVFire)))
				{
					for (u32 activeFireIndex = 0;
						activeFireIndex < cFire->activeFireCount;
						activeFireIndex++)
					{
						SAVFire *savFire = tempFires + activeFireIndex;

						Fire fire = {};
						fire.pos.x = savFire->x;
						fire.pos.y = savFire->y;
						addFireRaw(city, fire);
					}
					ASSERT((u32)layer->activeFireCount == cFire->activeFireCount);
				}
			}
			else if (identifiersAreEqual(header->identifier, SAV_HLTH_ID))
			{
				// Load Health
				CHECK_VERSION(HLTH);
				REQUIRE_META(HLTH);

				// u8 *startOfChunk = pos;
				SAVChunk_Health *cHealth = READ_CHUNK(SAVChunk_Health);
				cHealth = cHealth;

				// TODO: Implement Health!
			}
			else if (identifiersAreEqual(header->identifier, SAV_LVAL_ID))
			{
				// Load Land Value
				CHECK_VERSION(LVAL);
				REQUIRE_META(LVAL);

				u8 *startOfChunk = pos;
				SAVChunk_LandValue *cLandValue = READ_CHUNK(SAVChunk_LandValue);
				LandValueLayer *layer = &city->landValueLayer;

				decodeBlob(cLandValue->tileLandValue, startOfChunk, &layer->tileLandValue);
			}
			else if (identifiersAreEqual(header->identifier, SAV_PLTN_ID))
			{
				// Load Pollution
				CHECK_VERSION(PLTN);
				REQUIRE_META(PLTN);

				u8 *startOfChunk = pos;
				SAVChunk_Pollution *cPollution = READ_CHUNK(SAVChunk_Pollution);
				PollutionLayer *layer = &city->pollutionLayer;

				decodeBlob(cPollution->tilePollution, startOfChunk, &layer->tilePollution);
			}
			else if (identifiersAreEqual(header->identifier, SAV_TERR_ID))
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
				Array<u8> oldTypeToNewType = allocateArray<u8>(tempArena, cTerrain->terrainTypeCount + 1);
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
			}
			else if (identifiersAreEqual(header->identifier, SAV_TPRT_ID))
			{
				// Load Transport
				CHECK_VERSION(TPRT);
				REQUIRE_META(TPRT);

				// u8 *startOfChunk = pos;
				SAVChunk_Transport *cTransport = READ_CHUNK(SAVChunk_Transport);
				cTransport = cTransport;

				// TODO: Implement Transport!
			}
			else if (identifiersAreEqual(header->identifier, SAV_ZONE_ID))
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

	return succeeded;
}

bool fileHeaderIsValid(SAVFileHeader *fileHeader, String saveFileName)
{
	bool isValid = true;

	SAVFileHeader example = SAVFileHeader();
	if (!identifiersAreEqual(fileHeader->identifier, example.identifier))
	{
		logError("Save file '{0}' does not begin with the expected 4-byte sequence. Expected '{1}', got '{2}'"_s, {
			saveFileName,
			makeString((char*)example.identifier, 4),
			makeString((char*)fileHeader->identifier, 4)
		});
		isValid = false;
	}
	else if (!isMemoryEqual(&fileHeader->unixNewline, &example.unixNewline, 3))
	{
		logError("Save file '{0}' has corrupted newline characters. This probably means the saving or loading code is incorrect."_s, {
			saveFileName
		});
		isValid = false;
	}

	return isValid;
}

bool checkFileHeaderVersion(SAVFileHeader *fileHeader, String saveFileName)
{
	bool isValid = true;

	if (fileHeader->version > SAV_VERSION)
	{
		logError("Save file '{0}' was created with a newer save file format than we understand. File version is '{1}', maximum we support is '{2}'"_s, {
			saveFileName,
			formatInt(fileHeader->version),
			formatInt(SAV_VERSION),
		});
		isValid = false;
	}

	return isValid;
}

inline bool identifiersAreEqual(const u8 *a, const u8 *b)
{
	return (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]) && (a[3] == b[3]);
}

String readString(SAVString source, u8 *base)
{
	return makeString((char *)(base + source.relativeOffset), source.length, false);
}

// @Deprecated
String loadString(SAVString source, u8 *base, MemoryArena *arena)
{
	String toCopy = makeString((char *)(base + source.relativeOffset), source.length, false);

	return pushString(arena, toCopy);
}

void rleDecode(u8 *source, u8 *dest, s32 destSize)
{
	u8 *sourcePos = source;
	u8 *destPos = dest;
	u8 *destEnd = dest + destSize;

	while (destPos < destEnd)
	{
		s8 length = *((s8 *)sourcePos);
		sourcePos++;
		if (length < 0)
		{
			// Literals
			s8 literalCount = -length;
			copyMemory(sourcePos, destPos, literalCount);
			sourcePos += literalCount;
			destPos += literalCount;
		}
		else
		{
			// RLE
			u8 value = *sourcePos;
			sourcePos++;
			fillMemory(destPos, value, length);
			destPos += length;
		}
	}
}

SAVBlob appendBlob(s32 currentOffset, WriteBuffer *buffer, s32 length, u8 *data, SAVBlobCompressionScheme scheme)
{
	SAVBlob result = {};
	result.compressionScheme = scheme;
	result.relativeOffset = currentOffset;
	result.decompressedLength = length;

	switch (scheme)
	{
		case Blob_Uncompressed: {
			append(buffer, length, data);
			result.length = length;
		} break;

		case Blob_RLE_S8: {
			result.length = appendRLE(buffer, length, data);
		} break;

		default: {
			logError("Called appendBlob() with an unrecognized scheme! ({0}) Defaulting to Blob_Uncompressed."_s, {formatInt(scheme)});
			result = appendBlob(currentOffset, buffer, length, data, Blob_Uncompressed);
		} break;
	}

	return result;
}

SAVBlob appendBlob(s32 currentOffset, WriteBuffer *buffer, Array2<u8> *data, SAVBlobCompressionScheme scheme)
{
	return appendBlob(currentOffset, buffer, data->w * data->h, data->items, scheme);
}

bool decodeBlob(SAVBlob blob, u8 *baseMemory, u8 *dest, s32 destSize)
{
	bool succeeded = true;

	if ((u32)destSize < blob.decompressedLength)
	{
		logError("Unable to decode save file data blob because the destination is too small! (Need {0}, got {1})"_s, {formatInt(blob.decompressedLength), formatInt(destSize)});
		succeeded = false;
	}
	else
	{
		switch (blob.compressionScheme)
		{
			case Blob_Uncompressed: {
				copyMemory(baseMemory + blob.relativeOffset, dest, blob.length);
			} break;

			case Blob_RLE_S8: {
				rleDecode(baseMemory + blob.relativeOffset, dest, destSize);
			} break;

			default: {
				logError("Unable to decode save file data blob because the encoding is unrecognized! ({0})"_s, {formatInt(blob.compressionScheme)});
				succeeded = false;
			} break;
		}
	}

	return succeeded;
}

bool decodeBlob(SAVBlob blob, u8 *baseMemory, Array2<u8> *dest)
{
	return decodeBlob(blob, baseMemory, dest->items, dest->w * dest->h);
}
