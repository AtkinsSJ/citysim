#pragma once

bool writeSaveFile(City *city, FileHandle *file)
{
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
			SAVChunkHeader chunkHeader;
			s32 startOfChunkHeader = reserve(&buffer, sizeof(chunkHeader));
			copyMemory<u8>(SAV_META_ID, chunkHeader.identifier, 4);
			chunkHeader.version = SAV_META_VERSION;
			// TODO: function to fill in the chunkheader with the size automatically

			SAVChunk_Meta meta = {};
			meta.saveTimestamp = getCurrentUnixTimestamp();
			meta.cityWidth  = city->bounds.w;
			meta.cityHeight = city->bounds.h;
			meta.funds = city->funds;
			// meta.cityName = {city->name.length, };
			// meta.playerName = ;

			s32 metaLength = sizeof(meta);// + city->name.length;

			// Strings!

			// Fill the chunk header
			append(&buffer, sizeof(meta), &meta);
			// City name here

			chunkHeader.length = metaLength;
			overwriteAt(&buffer, startOfChunkHeader, sizeof(chunkHeader), &chunkHeader);
		}



		succeeded = writeToFile(file, &buffer);
	}

	return succeeded;
}
