#pragma once

char *readFileAsString(TemporaryMemoryArena *tempMemory, char *filename)
{
	void *fileData = 0;

	SDL_RWops *file = SDL_RWFromFile(filename, "r");
	if (file)
	{
		size_t fileLength = (size_t) file->seek(file, 0, RW_SEEK_END);
		file->seek(file, 0, RW_SEEK_SET);

		fileData = allocate(tempMemory, fileLength + 1); // 1 longer to ensure a trailing null for strings!

		if (fileData)
		{
			file->read(file, fileData, fileLength, 1);
		}

		file->close(file);
	}
	
	return (char *)fileData;
}

char *readFileAsString(MemoryArena *memory, char *filename)
{
	void *fileData = 0;

	SDL_RWops *file = SDL_RWFromFile(filename, "r");
	if (file)
	{
		size_t fileLength = (size_t) file->seek(file, 0, RW_SEEK_END);
		file->seek(file, 0, RW_SEEK_SET);

		fileData = allocate(memory, fileLength + 1); // 1 longer to ensure a trailing null for strings!

		if (fileData)
		{
			file->read(file, fileData, fileLength, 1);
		}

		file->close(file);
	}
	
	return (char *)fileData;
}

struct File
{
	size_t length;
	uint8 *data;
};

File readFile(TemporaryMemoryArena *tempMemory, char *filename)
{
	File result = {};

	SDL_RWops *file = SDL_RWFromFile(filename, "rb");
	if (file)
	{
		int64 length = file->seek(file, 0, RW_SEEK_END);
		file->seek(file, 0, RW_SEEK_SET);

		ASSERT(result.length <= INT32_MAX, "File is too big to fit into an int32!");

		result.length = (size_t) length;
		result.data = (uint8 *) allocate(tempMemory, result.length);

		if (result.data)
		{
			file->read(file, result.data, result.length, 1);
		}

		file->close(file);
	}
	
	return result;
}