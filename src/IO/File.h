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

// Returns the part of 'filename' after the final '.'
// eg, getFileExtension("foo.bar.baz") would return "baz".
// If there is no '.', we return an empty String.
StringView get_file_extension(StringView filename);
// Returns the part of filename WITHOUT the extension.
// So, the opposite of getFileExtension()
StringView get_file_name(StringView filename);
String constructPath(std::initializer_list<String> parts, bool appendWildcard = false);

// Locale-dependent files have the format "name.locale.extension"
Optional<StringView> get_file_locale_segment(StringView filename);

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
