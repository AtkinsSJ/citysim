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

		u32 cityTileCount = city->bounds.w * city->bounds.h;

		// Meta
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_META_ID, SAV_META_VERSION);

			SAVChunk_Meta meta = {};
			meta.saveTimestamp = getCurrentUnixTimestamp();
			meta.cityWidth  = (u16) city->bounds.w;
			meta.cityHeight = (u16) city->bounds.h;
			meta.funds = city->funds;
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

			SAVChunk_Terrain terr = {};
			s32 startOfTerr = reserve(&buffer, sizeof(terr));
			s32 offset = sizeof(terr);

			// Terrain types table
			terr.terrainTypeCount = terrainDefs.count;
			terr.offsetForTerrainTypeTable = offset;
			for (auto it = iterate(&terrainDefs); hasNext(&it); next(&it))
			{
				TerrainDef *def = get(it);
				u32 typeID = getIndex(it);
				u32 idLength = def->id.length;

				// 4 byte int id, 4 byte length, then the text as bytes
				appendStruct(&buffer, &typeID);
				appendStruct(&buffer, &idLength);
				append(&buffer, idLength, def->id.chars);

				offset += sizeof(typeID) + sizeof(idLength) + idLength;
			}

			// Tile terrain type (u8)
			terr.offsetForTileTerrainType = offset;
			append(&buffer, cityTileCount * sizeof(u8), layer->tileTerrainType);
			offset += cityTileCount * sizeof(u8);

			// Tile height (u8)
			terr.offsetForTileHeight = offset;
			append(&buffer, cityTileCount * sizeof(u8), layer->tileHeight);
			offset += cityTileCount * sizeof(u8);

			// Tile sprite offset (u8)
			terr.offsetForTileSpriteOffset = offset;
			append(&buffer, cityTileCount * sizeof(u8), layer->tileSpriteOffset);
			offset += cityTileCount * sizeof(u8);

			overwriteAt(&buffer, startOfTerr, sizeof(terr), &terr);
		}

		// Buildings
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_BLDG_ID, SAV_BLDG_VERSION);

			SAVChunk_Buildings bldg = {};
			s32 startOfBldg = reserve(&buffer, sizeof(bldg));
			s32 offset = sizeof(bldg);

			// Building types table
			bldg.buildingTypeCount = buildingCatalogue.allBuildings.count - 1; // Not the null def!
			bldg.offsetForBuildingTypeTable = offset;
			for (auto it = iterate(&buildingCatalogue.allBuildings); hasNext(&it); next(&it))
			{
				BuildingDef *def = get(it);
				if (def->typeID == 0) continue; // Skip the null building def!

				u32 typeID = def->typeID;
				u32 idLength = def->id.length;

				// 4 byte int id, 4 byte length, then the text as bytes
				appendStruct(&buffer, &typeID);
				appendStruct(&buffer, &idLength);
				append(&buffer, idLength, def->id.chars);

				offset += sizeof(typeID) + sizeof(idLength) + idLength;
			}

			// Highest ID
			bldg.highestBuildingID = city->highestBuildingID;

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
			bldg.buildingCount = city->buildings.count - 1; // Not the null building!
			bldg.offsetForBuildingArray = offset;
			for (auto it = iterate(&city->buildings); hasNext(&it); next(&it))
			{
				Building *building = get(&it);
				if (building->id == 0) continue; // Skip the null building!

				SAVBuilding sb = {};
				sb.id = building->id;
				sb.typeID = building->typeID;
				sb.x = (u16) building->footprint.x;
				sb.y = (u16) building->footprint.y;
				sb.w = (u16) building->footprint.w;
				sb.h = (u16) building->footprint.h;
				sb.spriteOffset = (u16) building->spriteOffset;
				sb.currentResidents = (u16) building->currentResidents;
				sb.currentJobs = (u16) building->currentJobs;

				appendStruct(&buffer, &sb);
				offset += sizeof(sb);
			}

			overwriteAt(&buffer, startOfBldg, sizeof(bldg), &bldg);
		}

		// Zones
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_ZONE_ID, SAV_ZONE_VERSION);
			ZoneLayer *layer = &city->zoneLayer;

			SAVChunk_Zone zone = {};
			s32 startOfZone = reserve(&buffer, sizeof(zone));
			s32 offset = sizeof(zone);

			// Tile zones
			zone.offsetForTileZone = offset;
			append(&buffer, cityTileCount * sizeof(u8), layer->tileZone);
			offset += cityTileCount * sizeof(u8);

			overwriteAt(&buffer, startOfZone, sizeof(zone), &zone);
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
			chunk.offsetForActiveFires = offset;
			// ughhhh I have to iterate the sectors to get this information!
			for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&layer->sectors); sectorIndex++)
			{
				FireSector *sector = getSectorByIndex(&layer->sectors, sectorIndex);

				for (auto it = iterate(&sector->activeFires); hasNext(&it); next(&it))
				{
					Fire *fire = get(it);
					SAVFire savFire = {};
					savFire.x = (u16) fire->pos.x;
					savFire.y = (u16) fire->pos.y;

					appendStruct(&buffer, &savFire);
				}
			}

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
			chunk.offsetForTileLandValue = offset;
			append(&buffer, cityTileCount * sizeof(u8), layer->tileLandValue);
			offset += cityTileCount * sizeof(u8);

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
			chunk.offsetForTilePollution = offset;
			append(&buffer, cityTileCount * sizeof(u8), layer->tilePollution);
			offset += cityTileCount * sizeof(u8);

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

		SAVFileHeader example = SAVFileHeader();
		if (!identifiersAreEqual(fileHeader->identifier, example.identifier))
		{
			logError("Save file '{0}' does not begin with the expected 4-byte sequence. Expected '{1}', got '{2}'"_s, {
				saveFile.name,
				makeString((char*)example.identifier, 4),
				makeString((char*)fileHeader->identifier, 4)
			});
			succeeded = false;
			break;
		}
		if (fileHeader->version > example.version)
		{
			logError("Save file '{0}' was created with a newer save file format than we understand. File version is '{1}', maximum we support is '{2}'"_s, {
				saveFile.name,
				formatInt(fileHeader->version),
				formatInt(example.version),
			});
			succeeded = false;
			break;
		}
		if (!isMemoryEqual(&fileHeader->unixNewline, &example.unixNewline, 3))
		{
			logError("Save file '{0}' has corrupted newline characters. This probably means the saving or loading code is incorrect."_s, {
				saveFile.name
			});
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

			if (identifiersAreEqual(header->identifier, SAV_META_ID))
			{
				// Load Meta
				CHECK_VERSION(META);

				u8 *startOfChunk = pos;
				SAVChunk_Meta *meta = READ_CHUNK(SAVChunk_Meta);

				String cityName = loadString(meta->cityName, startOfChunk, gameArena);
				String playerName = loadString(meta->playerName, startOfChunk, gameArena);
				initCity(gameArena, city, meta->cityWidth, meta->cityHeight, cityName, playerName, meta->funds);
				cityTileCount = city->bounds.w * city->bounds.h;

				hasLoadedMeta = true;
			}
			else if (identifiersAreEqual(header->identifier, SAV_BDGT_ID))
			{
				// Load Budget
				CHECK_VERSION(BDGT);
				REQUIRE_META(BDGT);

				u8 *startOfChunk = pos;
				SAVChunk_Budget *budget = READ_CHUNK(SAVChunk_Budget);

				// TODO: Implement!
			}
			else if (identifiersAreEqual(header->identifier, SAV_BLDG_ID))
			{
				// Load Building
				CHECK_VERSION(BLDG);
				REQUIRE_META(BLDG);

				u8 *startOfChunk = pos;
				SAVChunk_Buildings *buildings = READ_CHUNK(SAVChunk_Buildings);

				city->highestBuildingID = buildings->highestBuildingID;

				// TODO: Map the file's building type IDs to the game's ones @MapIDs
				// (This is related to the general "the game needs to map IDs when data files change" thing.)

				SAVBuilding *savBuilding = (SAVBuilding *)(startOfChunk + buildings->offsetForBuildingArray);
				for (u32 buildingIndex = 0;
					buildingIndex < buildings->buildingCount;
					buildingIndex++, savBuilding++)
				{
					Rect2I footprint = irectXYWH(savBuilding->x, savBuilding->y, savBuilding->w, savBuilding->h);
					BuildingDef *def = getBuildingDef(savBuilding->typeID);
					Building *building = addBuildingDirect(city, savBuilding->id, def, footprint);
					building->spriteOffset     = savBuilding->spriteOffset;
					building->currentResidents = savBuilding->currentResidents;
					building->currentJobs      = savBuilding->currentJobs;
				}
			}
			else if (identifiersAreEqual(header->identifier, SAV_CRIM_ID))
			{
				// Load Crime
				CHECK_VERSION(CRIM);
				REQUIRE_META(CRIM);

				u8 *startOfChunk = pos;
				SAVChunk_Crime *crime = READ_CHUNK(SAVChunk_Crime);

				// TODO: Implement Crime!
			}
			else if (identifiersAreEqual(header->identifier, SAV_EDUC_ID))
			{
				// Load Education
				CHECK_VERSION(EDUC);
				REQUIRE_META(EDUC);

				u8 *startOfChunk = pos;
				SAVChunk_Education *education = READ_CHUNK(SAVChunk_Education);

				// TODO: Implement Education!
			}
			else if (identifiersAreEqual(header->identifier, SAV_FIRE_ID))
			{
				// Load Fire
				CHECK_VERSION(FIRE);
				REQUIRE_META(FIRE);

				u8 *startOfChunk = pos;
				SAVChunk_Fire *fire = READ_CHUNK(SAVChunk_Fire);

				// TODO: Implement Fire!
			}
			else if (identifiersAreEqual(header->identifier, SAV_HLTH_ID))
			{
				// Load Health
				CHECK_VERSION(HLTH);
				REQUIRE_META(HLTH);

				u8 *startOfChunk = pos;
				SAVChunk_Health *health = READ_CHUNK(SAVChunk_Health);

				// TODO: Implement Health!
			}
			else if (identifiersAreEqual(header->identifier, SAV_LVAL_ID))
			{
				// Load Land Value
				CHECK_VERSION(LVAL);
				REQUIRE_META(LVAL);

				u8 *startOfChunk = pos;
				SAVChunk_LandValue *landValue = READ_CHUNK(SAVChunk_LandValue);

				// TODO: Implement LandValue!
			}
			else if (identifiersAreEqual(header->identifier, SAV_PLTN_ID))
			{
				// Load Pollution
				CHECK_VERSION(PLTN);
				REQUIRE_META(PLTN);

				u8 *startOfChunk = pos;
				SAVChunk_Pollution *pollution = READ_CHUNK(SAVChunk_Pollution);

				// TODO: Implement Pollution!
			}
			else if (identifiersAreEqual(header->identifier, SAV_TERR_ID))
			{
				// Load Terrain
				CHECK_VERSION(TERR);
				REQUIRE_META(TERR);

				u8 *startOfChunk = pos;
				SAVChunk_Terrain *terrain = READ_CHUNK(SAVChunk_Terrain);
				TerrainLayer *layer = &city->terrainLayer;

				// TODO: Map the file's terrain type IDs to the game's ones @MapIDs
				// (This is related to the general "the game needs to map IDs when data files change" thing.)

				u8 *tileTerrainType  = startOfChunk + terrain->offsetForTileTerrainType;
				copyMemory(tileTerrainType, layer->tileTerrainType, cityTileCount);

				u8 *tileHeight       = startOfChunk + terrain->offsetForTileHeight;
				copyMemory(tileHeight, layer->tileHeight, cityTileCount);

				u8 *tileSpriteOffset = startOfChunk + terrain->offsetForTileSpriteOffset;
				copyMemory(tileSpriteOffset, layer->tileSpriteOffset, cityTileCount);
			}
			else if (identifiersAreEqual(header->identifier, SAV_TPRT_ID))
			{
				// Load Transport
				CHECK_VERSION(TPRT);
				REQUIRE_META(TPRT);

				u8 *startOfChunk = pos;
				SAVChunk_Transport *transport = READ_CHUNK(SAVChunk_Transport);

				// TODO: Implement Transport!
			}
			else if (identifiersAreEqual(header->identifier, SAV_ZONE_ID))
			{
				// Load Zone
				CHECK_VERSION(ZONE);
				REQUIRE_META(ZONE);

				u8 *startOfChunk = pos;
				SAVChunk_Zone *zone = READ_CHUNK(SAVChunk_Zone);

				// TODO: Implement Zone!
			}
		}

		#undef READ

		// This break is because we succeeded!
		succeeded = true;
		break;
	}

	return succeeded;
}

inline bool identifiersAreEqual(const u8 *a, const u8 *b)
{
	return (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]) && (a[3] == b[3]);
}

String loadString(SAVString source, u8 *base, MemoryArena *arena)
{
	String toCopy = makeString((char *)(base + source.relativeOffset), source.length, false);

	return pushString(arena, toCopy);
}