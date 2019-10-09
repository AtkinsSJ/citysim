#pragma once

// Returns the part of 'filename' after the final '.'
// eg, getFileExtension("foo.bar.baz") would return "baz".
// If there is no '.', we return an empty String.
String getFileExtension(String filename)
{
	String fileExtension = {};

	s32 length = 0;
	while ((length < filename.length) && (filename[filename.length - length - 1] != '.'))
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

String constructPath(std::initializer_list<String> parts, bool appendWildcard)
{
	return platform_constructPath(parts, appendWildcard);
}

String getFileLocale(String filename)
{
	String result = {};

	Maybe<s32> endPos = findIndexOfChar(filename, '.', true);
	if (endPos.isValid)
	{
		Maybe<s32> startPos = findIndexOfChar(filename, '.', true, endPos.value - 1);
		if (startPos.isValid)
		{
			result = makeString(filename.chars + startPos.value + 1, endPos.value - startPos.value - 1);
		}
	}

	return result;
}

FileHandle openFile(String path, FileAccessMode mode)
{
	DEBUG_FUNCTION();
	
	ASSERT(isNullTerminated(path)); //openFile() path must be null-terminated.

	FileHandle result = {};

	result.path = path;
	result.sdl_file = SDL_RWFromFile(path.chars, fileAccessModeStrings[mode]);
	result.isOpen = (result.sdl_file != null);

	return result;
}

void closeFile(FileHandle *file)
{
	DEBUG_FUNCTION();
	
	if (file->isOpen)
	{
		SDL_RWclose(file->sdl_file);
		file->isOpen = false;
	}
}

s64 getFilePosition(FileHandle *file)
{
	s64 result = -1;

	if (file->isOpen)
	{
		result = SDL_RWtell(file->sdl_file);
	}

	return result;
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
	DEBUG_FUNCTION();
	
	smm bytesRead = 0;

	if (file->isOpen)
	{
		bytesRead = SDL_RWread(file->sdl_file, memory, 1, size);
	}

	return bytesRead;
}

File readFile(MemoryArena *memoryArena, String filePath)
{
	DEBUG_FUNCTION();
	
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
			logWarn("File '{0}' was only partially read. Size {1}, read {2}"_s, {filePath, formatInt(fileSize), formatInt(bytesRead)});
		}
		else
		{
			result.isLoaded = true;
		}

		closeFile(&handle);
	}
	else
	{
		logWarn("Failed to open file '{0}' for reading."_s, {filePath});
	}

	return result;
}

bool writeFile(String filePath, String contents)
{
	DEBUG_FUNCTION();
	
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
		logError("Failed to open file '{0}' for writing."_s, {filePath});
	}

	return succeeded;
}

template<typename T>
bool writeToFile(FileHandle *file, T *data)
{
	bool succeeded = false;

	if (file->isOpen)
	{
		smm bytesWritten = SDL_RWwrite(file->sdl_file, data, 1, sizeof(T));
		succeeded = (bytesWritten == sizeof(T));
	}
	else
	{
		logError("Cannot write to file '{0}'."_s, {file->path});
	}

	return succeeded;
}

DirectoryListingHandle beginDirectoryListing(String path, FileInfo *result)
{
	return platform_beginDirectoryListing(path, result);
}

bool nextFileInDirectory(DirectoryListingHandle *handle, FileInfo *result)
{
	bool foundAResult = false;
	if (handle->isValid)
	{
		foundAResult = platform_nextFileInDirectory(handle, result);
	}
	return foundAResult;
}

void stopDirectoryListing(DirectoryListingHandle *handle)
{
	if (handle->isValid)
	{
		platform_stopDirectoryListing(handle);
		handle->isValid = false;
	}
}

inline bool hasNextFile(iterateDirectoryListing *iterator)
{
	return iterator->handle.isValid;
}

inline void findNextFile(iterateDirectoryListing *iterator)
{
	nextFileInDirectory(&iterator->handle, &iterator->fileInfo);
}

inline FileInfo *getFileInfo(iterateDirectoryListing *iterator)
{
	return &iterator->fileInfo;
}


DirectoryChangeWatchingHandle beginWatchingDirectory(String path)
{
	return platform_beginWatchingDirectory(path);
}

bool hasDirectoryChanged(DirectoryChangeWatchingHandle *handle)
{
	bool result = false;
	if (handle->isValid)
	{
		result = platform_hasDirectoryChanged(handle);
	}
	return result;
}

void stopWatchingDirectory(DirectoryChangeWatchingHandle *handle)
{
	if (handle->isValid)
	{
		platform_stopWatchingDirectory(handle);
		handle->isValid = false;
	}
}
