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

struct DirectoryListingHandle
{
	bool isValid;
	u32 errorCode;
	String path;

	union
	{
		struct
		{
			HANDLE hFile;
		} windows;
	};
};

enum FileFlags
{
	FileFlag_Directory = 1 << 0,
	FileFlag_Hidden    = 1 << 1,
	FileFlag_ReadOnly  = 1 << 2,
};

struct FileInfo
{
	String filename;
	u32 flags;
	smm size;
};

struct DirectoryChangeWatchingHandle
{
	bool isValid;
	u32 errorCode;
	String path;

	union
	{
		struct
		{
			HANDLE handle;
		} windows;
	};
};

// Returns the part of 'filename' after the final '.'
// eg, getFileExtension("foo.bar.baz") would return "baz".
// If there is no '.', we return an empty String.
String getFileExtension(String filename);
String constructPath(std::initializer_list<String> parts, bool appendWildcard=false);

FileHandle openFile(String path, FileAccessMode mode);
void closeFile(FileHandle *file);
smm getFileSize(FileHandle *file);

File readFile(MemoryArena *memoryArena, String filePath);
smm readFileIntoMemory(FileHandle *file, smm size, u8 *memory);
// Reads the entire file into a Blob that's allocated in temporary memory.
// If you want to refer to parts of it later, you need to copy the data somewhere else!
inline Blob readTempFile(String filePath)
{
	return readFile(globalFrameTempArena, filePath).data;
}

bool writeFile(String filePath, String contents);

DirectoryListingHandle beginDirectoryListing(String path, FileInfo *result);
bool nextFileInDirectory(DirectoryListingHandle *handle, FileInfo *result);

DirectoryChangeWatchingHandle beginWatchingDirectory(String path);
bool hasDirectoryChanged(DirectoryChangeWatchingHandle *handle);
void stopWatchingDirectory(DirectoryChangeWatchingHandle *handle);
