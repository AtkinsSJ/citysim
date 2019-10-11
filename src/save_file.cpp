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

			copyMemory<u8>(chunkID, chunkHeader.identifier, 4);
			chunkHeader.version = chunkVersion;
		}

		~ChunkHeaderWrapper()
		{
			chunkHeader.length = getCurrentPosition(buffer) - this->startOfChunkData;
			overwriteAt(buffer, startOfChunkHeader, sizeof(chunkHeader), &chunkHeader);
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
			meta.cityWidth  = city->bounds.w;
			meta.cityHeight = city->bounds.h;
			meta.funds = city->funds;
			s32 stringOffset = sizeof(meta);
			meta.cityName = {(u32)city->name.length, (u32)stringOffset};
			stringOffset += meta.cityName.length;
			meta.playerName = {(u32)city->playerName.length, (u32)stringOffset};
			stringOffset += meta.playerName.length;

			// Write the data
			append(&buffer, sizeof(meta), &meta);
			append(&buffer, city->name.length, city->name.chars);
			append(&buffer, city->playerName.length, city->playerName.chars);
		}

		// Terrain
		{
			ChunkHeaderWrapper wrapper(&buffer, SAV_TERR_ID, SAV_TERR_VERSION);

			SAVChunk_Terrain terr = {};

			append(&buffer, sizeof(terr), &terr);
		}

		succeeded = writeToFile(file, &buffer);
	}

	return succeeded;
}
