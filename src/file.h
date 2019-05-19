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
	
	Blob data;
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

// Returns the part of 'filename' after the final '.'
// eg, getFileExtension("foo.bar.baz") would return "baz".
// If there is no '.', we return an empty String.
String getFileExtension(String filename)
{
	String fileExtension = {};

	s32 length = 0;
	while ((length < filename.length) && (filename.chars[filename.length - length - 1] != '.'))
	{
		length++;
	}

	if (length == filename.length)
	{
		// no extension!
		length = 0;
	}

	fileExtension = makeString(filename.chars + filename.length - length, length);

	return fileExtension;
}

FileHandle openFile(String path, FileAccessMode mode)
{
	ASSERT(isNullTerminated(path), "openFile() path must be null-terminated.");

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
	smm fileSize = -1;

	if (file->isOpen)
	{
		s64 currentPos = SDL_RWtell(file->sdl_file);

		fileSize = (smm) SDL_RWseek(file->sdl_file, 0, RW_SEEK_END);
		SDL_RWseek(file->sdl_file, currentPos, RW_SEEK_SET);
	}

	return fileSize;
}

smm readFileIntoMemory(FileHandle *file, smm size, u8 *memory)
{
	smm bytesRead = 0;

	if (file->isOpen)
	{
		bytesRead = SDL_RWread(file->sdl_file, memory, 1, size);
	}

	return bytesRead;
}

File readFile(MemoryArena *memoryArena, String filePath)
{
	File result = {};
	result.name = filePath;
	result.isLoaded = false;

	FileHandle handle = openFile(filePath, FileAccess_Read);

	if (handle.isOpen)
	{
		smm fileSize = getFileSize(&handle);
		result.data = allocateBlob(memoryArena, fileSize);
		smm bytesRead = readFileIntoMemory(&handle, fileSize, result.data.memory);

		if (bytesRead != fileSize)
		{
			logWarn("File '{0}' was only partially read. Size {1}, read {2}", {filePath, formatInt(fileSize), formatInt(bytesRead)});
		}
		else
		{
			result.isLoaded = true;
		}

		closeFile(&handle);
	}
	else
	{
		logError("Failed to open file '{0}' for reading.", {filePath});
	}

	return result;
}

// Reads the entire file into a Blob that's allocated in temporary memory.
// If you want to refer to parts of it later, you need to copy the data somewhere else!
inline Blob readTempFile(String filePath)
{
	return readFile(globalFrameTempArena, filePath).data;
}

bool writeFile(String filePath, String contents)
{
	bool succeeded = false;

	FileHandle file = openFile(filePath, FileAccess_Write);

	if (file.isOpen)
	{
		smm bytesWritten = SDL_RWwrite(file.sdl_file, contents.chars, 1, contents.length);
		succeeded = (bytesWritten == contents.length);
		closeFile(&file);
	}
	else
	{
		logError("Failed to open file '{0}' for writing.", {filePath});
	}

	return succeeded;
}
