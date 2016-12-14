#pragma once

char *readFileAsString(MemoryArena *memory, char *filename)
{
	void *fileData = 0;

	SDL_RWops *file = SDL_RWFromFile(filename, "r");
	if (file)
	{
		umm fileLength = (umm) file->seek(file, 0, RW_SEEK_END);
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
	umm length;
	uint8 *data;
};

File readFile(MemoryArena *arena, char *filename)
{
	File result = {};

	SDL_RWops *file = SDL_RWFromFile(filename, "rb");
	if (file)
	{
		int64 length = file->seek(file, 0, RW_SEEK_END);
		file->seek(file, 0, RW_SEEK_SET);

		ASSERT(result.length <= INT32_MAX, "File is too big to fit into an int32!");

		result.length = (umm) length;
		result.data = PushArray(arena, uint8, result.length);

		if (result.data)
		{
			file->read(file, result.data, result.length, 1);
		}

		file->close(file);
	}
	
	return result;
}