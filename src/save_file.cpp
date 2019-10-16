#pragma once

bool writeSaveFile(City *city, FileHandle *file)
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
				u32 idLength = def->id.length;

				// 4 byte length, then the text as bytes
				appendStruct(&buffer, &idLength);
				append(&buffer, idLength, def->id.chars);

				offset += sizeof(idLength) + idLength;
			}

			// Tile terrain type (u8)
			terr.offsetForTileTerrainType = offset;
			append(&buffer, cityTileCount * sizeof(u8), layer->tileTerrainType);
			offset += cityTileCount * sizeof(u8);

			// Tile height (u8)
			terr.offsetForTileHeight = offset;
			append(&buffer, cityTileCount * sizeof(u8), layer->tileTerrainHeight);
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
			bldg.buildingTypeCount = buildingCatalogue.allBuildings.count;
			bldg.offsetForBuildingTypeTable = offset;
			for (auto it = iterate(&buildingCatalogue.allBuildings); hasNext(&it); next(&it))
			{
				BuildingDef *def = get(it);
				u32 typeID = def->typeID;
				u32 idLength = def->id.length;

				// 4 byte int id, 4 byte length, then the text as bytes
				appendStruct(&buffer, &typeID);
				appendStruct(&buffer, &idLength);
				append(&buffer, idLength, def->id.chars);

				offset += sizeof(typeID) + sizeof(idLength) + idLength;
			}

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
			bldg.buildingCount = city->buildings.count;
			bldg.offsetForBuildingArray = offset;
			for (auto it = iterate(&city->buildings); hasNext(&it); next(&it))
			{
				Building *building = get(&it);

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


		succeeded = writeToFile(file, &buffer);
	}

	return succeeded;
}
