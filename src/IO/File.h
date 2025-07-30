/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Debug/Debug.h>
#include <SDL2/SDL_rwops.h>
#include <Util/Basic.h>
#include <Util/Flags.h>
#include <Util/Log.h>
#include <Util/Memory.h>
#include <Util/MemoryArena.h>
#include <Util/String.h>

#ifdef __linux__
#    include <dirent.h>
#endif

struct FileHandle {
    String path;
    bool isOpen;
    SDL_RWops* sdl_file;
};

struct File {
    String name;
    bool isLoaded;

    Blob data;
};

enum class FileAccessMode : u8 {
    Read,
    Write,
    COUNT,
};

static EnumMap<FileAccessMode, String> file_access_mode_strings {
    "rb"_s,
    "wb"_s,
};

struct DirectoryListingHandle {
    bool isValid;
    u32 errorCode;
    String path;

#ifdef __linux__
    DIR* dir;
    int dirFD;
#else
    struct
    {
        HANDLE hFile;
    } windows;
#endif
};

enum class FileFlags : u8 {
    Directory,
    Hidden,
    ReadOnly,
    COUNT,
};

struct FileInfo {
    String filename;
    Flags<FileFlags> flags;
    smm size;
};

struct DirectoryChangeWatchingHandle {
    bool isValid;
    u32 errorCode;
    String path;

#ifdef __linux__
    int inotify_fd;
    int watcher_fd;
#else
    struct
    {
        HANDLE handle;
    } windows;
#endif
};

// Returns the part of 'filename' after the final '.'
// eg, getFileExtension("foo.bar.baz") would return "baz".
// If there is no '.', we return an empty String.
String getFileExtension(String filename);
// Returns the part of filename WITHOUT the extension.
// So, the opposite of getFileExtension()
String getFileName(String filename);
String constructPath(std::initializer_list<String> parts, bool appendWildcard = false);

// Locale-dependent files have the format "name.locale.extension"
// If the filename doesn't follow this format, returns an empty String.
String getFileLocale(String filename);

FileHandle openFile(String path, FileAccessMode mode);
void closeFile(FileHandle* file);
smm getFileSize(FileHandle* file);
s64 getFilePosition(FileHandle* file);
bool deleteFile(String path);

bool createDirectory(String path);

File readFile(FileHandle* file, MemoryArena* arena = &temp_arena());
File readFile(MemoryArena* memoryArena, String filePath);
// Reads the entire file into a Blob that's allocated in temporary memory.
// If you want to refer to parts of it later, you need to copy the data somewhere else!
inline Blob readTempFile(String filePath)
{
    return readFile(&temp_arena(), filePath).data;
}

// Returns how much was read
smm readFromFile(FileHandle* file, Blob& dest);

// Attempts to read `size` bytes from position `position`, and copies them to `result`.
// If there are fewer than `size` bytes remaining, reads all it can. Returns how many bytes were read.
smm readData(FileHandle* file, smm position, smm size, void* result);

template<typename T>
bool readStruct(FileHandle* file, smm position, T* result)
{
    DEBUG_FUNCTION();

    bool succeeded = true;

    smm byteLength = sizeof(T);
    smm bytesRead = readData(file, position, byteLength, result);
    if (bytesRead != byteLength) {
        succeeded = false;
        logWarn("Failed to read struct '{0}' from file '{1}'"_s, { typeNameOf<T>(), file->path });
    }

    return succeeded;
}

template<typename T>
bool readArray(FileHandle* file, smm position, s32 count, Array<T>* result)
{
    DEBUG_FUNCTION();

    bool succeeded = false;

    if (result->capacity >= count) {
        smm byteLength = sizeof(T) * count;
        smm bytesRead = readData(file, position, byteLength, result->items);
        if (bytesRead == byteLength) {
            result->count = count;
            succeeded = true;
        } else {
            logWarn("Failed to read array of '{0}', length {1}, from file '{2}': Reached end of file"_s, { typeNameOf<T>(), formatInt(count), file->path });
        }
    } else {
        logError("Failed to read array of '{0}', length {1}, from file '{2}': result parameter too small"_s, { typeNameOf<T>(), formatInt(count), file->path });
    }

    return succeeded;
}

bool writeFile(String filePath, String contents);

bool writeToFile(FileHandle* file, smm dataLength, void* data);

template<typename T>
bool writeToFile(FileHandle* file, T* data)
{
    return writeToFile(file, sizeof(T), data);
}

/*
 * Lists the files and folders in a directory, one at a time. This is NOT recursive!
 * Start with beginDirectoryListing(), then call nextFileInDirectory() to get subsequent results.
 * The handle is automatically closed when you reach the end of the directory listing, or you can
 * call stopDirectoryListing() to finish early.
 *
 * nextFileInDirectory() checks the handle is valid, and returns false if not.
 *
 * - Sam, 30/05/2019
 */
DirectoryListingHandle beginDirectoryListing(String path, FileInfo* result);
bool nextFileInDirectory(DirectoryListingHandle* handle, FileInfo* result);
void stopDirectoryListing(DirectoryListingHandle* handle);

/*
 * Lists the files and folders in a directory, one at a time. This is NOT recursive!
 * Iterator-style interface for directory listing, because that better fits my mental model for how
 * it should work. I'm now wondering if we even need the "original" one, above?
 * Use:

        for (auto it = iterateDirectoryListing(pathToScan);
                hasNextFile(&it);
                findNextFile(&it))
        {
                FileInfo *fileInfo = getFileInfo(&it);
        }

 * The handle and related stuff is closed and cleaned-up automatically at the end of scope.
 * (So, whenever the for-loop ends, however that happens.)
 * NB: The pointer returned by getFileInfo() is only valid until the next call to findNextFile() or
 * the iterator goes out of scope!
 */
struct iterateDirectoryListing {
    DirectoryListingHandle handle;
    FileInfo fileInfo;

    iterateDirectoryListing(String path)
    {
        handle = beginDirectoryListing(path, &fileInfo);
    }

    ~iterateDirectoryListing()
    {
        stopDirectoryListing(&handle);
    }
};
bool hasNextFile(iterateDirectoryListing* iterator);
void findNextFile(iterateDirectoryListing* iterator);
FileInfo* getFileInfo(iterateDirectoryListing* iterator);

/*
 * Watch for file changes within a specific directory.
 * Right now, this watches for any file or directory changes within that directory, recursively.
 * Call beginWatchingDirectory() with the path, then hasDirectoryChanged() to see if anything has
 * changed since the last call. If you want to, you can close the handle with stopWatchingDirectory().
 *
 * hasDirectoryChanged() checks if the handle is valid, and returns false (for "no changes") if not,
 * so it's safe to call it on platforms that this system isn't implemented on, or in cases where the
 * beginWatchingDirectory() call failed for whatever reason. It Just Works!™
 *
 * - Sam, 30/05/2019
 */
DirectoryChangeWatchingHandle beginWatchingDirectory(String path);
bool hasDirectoryChanged(DirectoryChangeWatchingHandle* handle);
void stopWatchingDirectory(DirectoryChangeWatchingHandle* handle);
