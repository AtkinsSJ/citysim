#pragma once

String readFileAsString(MemoryArena *memory, String filename)
{
	String result = {};

	SDL_RWops *file = SDL_RWFromFile(filename.chars, "r");
	if (file)
	{
		umm fileLength = (umm) file->seek(file, 0, RW_SEEK_END);
		file->seek(file, 0, RW_SEEK_SET);

 		// 1 longer to ensure a trailing null for strings!
		result = newString(memory, fileLength + 1);
		result.chars[fileLength] = 0;

		if (result.chars)
		{
			file->read(file, result.chars, fileLength, 1);
			result.length = fileLength;
		}

		file->close(file);

	}
	
	return result;
}

/*
 * @Deprecated This File struct can probably vanish now, because it's identical to a String.
 */

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