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
			meta.cityWidth  = city->bounds.w;
			meta.cityHeight = city->bounds.h;
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

		succeeded = writeToFile(file, &buffer);
	}

	return succeeded;
}
