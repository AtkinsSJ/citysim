#pragma once

struct File
{
	String name;
	bool isLoaded;
	
	smm length;
	u8* data;
};

File readFile(MemoryArena *memory, String filename)
{
	File result = {};
	result.name = filename;
	result.isLoaded = false;

	SDL_RWops *file = SDL_RWFromFile(filename.chars, "rb");
	if (file)
	{
		smm fileLength = (smm) file->seek(file, 0, RW_SEEK_END);
		file->seek(file, 0, RW_SEEK_SET);

		result.data = PushArray(memory, u8, fileLength);

		if (result.data)
		{
			file->read(file, result.data, fileLength, 1);
			result.length = fileLength;
			result.isLoaded = true;
		}

		file->close(file);

	}
	
	return result;
}

String readFileAsString(MemoryArena *memory, String filename)
{
	File file = readFile(memory, filename);
	String result = makeString((char*)file.data, file.length);

	return result;
}
