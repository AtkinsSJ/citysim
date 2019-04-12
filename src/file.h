#pragma once

struct FileHandle
{
	bool isOpen;
	SDL_RWops *sdl_file;
};

struct File
{
	String name;
	bool isLoaded;
	
	smm length;
	u8* data;
};

enum FileAccessMode
{
	FileAccess_Read,
	FileAccess_Write
};

char *fileAccessModeStrings[] = {
	"rb",
	"wb"
};

FileHandle openFile(String path, FileAccessMode mode)
{
	ASSERT(!path.isNullTerminated, "openFile() path must be null-terminated.");

	FileHandle result = {};

	result.sdl_file = SDL_RWFromFile(path.chars, fileAccessModeStrings[mode]);
	result.isOpen = (result.sdl_file != null);

	return result;
}

void closeFile(FileHandle *file)
{
	if (file->isOpen)
	{
		SDL_RWclose(file->sdl_file);
		file->isOpen = false;
	}
}

smm getFileSize(FileHandle *file)
{
	s64 currentPos = SDL_RWtell(file->sdl_file);

	smm fileSize = (smm) SDL_RWseek(file->sdl_file, 0, RW_SEEK_END);
	SDL_RWseek(file->sdl_file, currentPos, RW_SEEK_SET);

	return fileSize;
}

smm readFileIntoMemory(FileHandle *file, smm size, u8 *memory)
{
	smm bytesRead = 0;

	bytesRead = SDL_RWread(file->sdl_file, memory, 1, size);

	return bytesRead;
}

// TODO: Switch to FileHandle!
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

// TODO: Switch to FileHandle!
String readFileAsString(MemoryArena *memory, String filename)
{
	File file = readFile(memory, filename);
	String result = makeString((char*)file.data, file.length);

	return result;
}

// TODO: Switch to FileHandle!
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
					{filename, makeString(SDL_GetError()), formatInt(writeLength), formatInt(contents.length)});
		}
		SDL_RWclose(file);
	}

	return succeeded;
}
