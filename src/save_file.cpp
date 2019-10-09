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
		append(buffer, sizeof(fileHeader), &fileHeader);

		// Meta
		

		succeeded = writeToFile(file, &buffer);
	}

	// I'm starting to think what we want is a StringBuilder style thing here,
	// where we just throw bytes at it and it grows automatically, and we can
	// then modify past parts of it easily and quickly. Having to seek around
	// a file on disk is a bit messy, and the variable-length strings and stuff
	// make it really fiddly (and error-prone) to just "prepare data->write data"
	// without ever going back and modifying previous parts. (I mean, a
	// fundamental part of how this works is stating at the beginning of a chunk
	// how large it is!)
	// - Sam, 09/10/2019


	// Meta
	if (succeeded)
	{
		// ChunkHeader
		s64 chunkHeaderStart = getFilePosition(file);
		SAVChunkHeader chunkHeader = {};
		succeeded = writeToFile<SAVChunkHeader>(file, &chunkHeader);

		if (succeeded)
		{
			s64 metaStart = getFilePosition(file);
			SAVChunk_Meta meta = {};
			meta.saveTimestamp = 0; // TODO: Timestamp!
			meta.cityWidth  = city->bounds.x;
			meta.cityHeight = city->bounds.y;
			meta.funds = city->funds;
			meta.cityName = {city->name.length, };
			// meta.playerName = ;

			succeeded = writeToFile(file, &meta, );//

			// Now, go back and write the header data
		}
	}

	return succeeded;
}
