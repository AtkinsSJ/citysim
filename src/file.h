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
		smm fileLength = (smm) SDL_RWseek(file, 0, RW_SEEK_END);
		SDL_RWseek(file, 0, RW_SEEK_SET);

		result.data = PushArray(memory, u8, fileLength);

		if (result.data)
		{
			SDL_RWread(file, result.data, fileLength, 1);
			result.length = fileLength;
			result.isLoaded = true;
		}

		SDL_RWclose(file);

	}
	
	return result;
}

String readFileAsString(MemoryArena *memory, String filename)
{
	File file = readFile(memory, filename);
	String result = makeString((char*)file.data, file.length);

	return result;
}

bool writeFile(String filename, String contents)
{
	bool succeeded = false;

	SDL_RWops *file = SDL_RWFromFile(filename.chars, "wb");
	if (file)
	{
		smm writeLength = SDL_RWwrite(file, contents.chars, 1, contents.length);
		if (writeLength == contents.length)
		{
			succeeded = true;
		}
		else
		{
			logError("Error while writing file {0}: {1} ({2} of {3} bytes written)",
					{filename, stringFromChars(SDL_GetError()), formatInt(writeLength), formatInt(contents.length)});
		}
		SDL_RWclose(file);
	}

	return succeeded;
}
