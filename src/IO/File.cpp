/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "File.h"
#include "../platform.h"
#include <Debug/Debug.h>
#include <Util/Locale.h>
#include <Util/Log.h>

// Returns the part of 'filename' after the final '.'
// eg, getFileExtension("foo.bar.baz") would return "baz".
// If there is no '.', we return an empty String.
StringView get_file_extension(StringView filename)
{
    // FIXME: Use find() and substring(). Also return StringView.
    auto length = 0u;
    while ((length < filename.length()) && (filename[filename.length() - length - 1] != '.')) {
        length++;
    }

    if (length == filename.length()) {
        // no extension!
        return {};
    }

    return filename.substring(filename.length() - length, length);
}

StringView get_file_name(StringView filename)
{
    // FIXME: Use find() and substring(). Also return StringView.
    auto length = 0u;
    while (length < filename.length() && filename[filename.length() - length - 1] != '.') {
        length++;
    }

    if (length == filename.length()) {
        // no extension!
        return filename;
    }
    return filename.substring(0, filename.length() - length - 1);
}

String constructPath(std::initializer_list<String> parts, bool appendWildcard)
{
    return platform_constructPath(parts, appendWildcard);
}

Optional<StringView> get_file_locale_segment(StringView filename)
{
    // FIXME: Use find() and substring().
    // The locale is a section just before the file extension, eg `foo.en.txt`
    if (auto end_position = filename.find('.', SearchFrom::End); end_position.has_value()) {
        if (auto start_position = filename.find('.', SearchFrom::End, end_position.value() - 1); start_position.has_value()) {
            return filename.substring(start_position.value() + 1, end_position.value() - start_position.value() - 1);
        }
    }

    return {};
}

FileHandle openFile(String path, FileAccessMode mode)
{
    DEBUG_FUNCTION();

    ASSERT(path.is_null_terminated()); // openFile() path must be null-terminated.

    FileHandle result = {};

    result.path = path;
    result.sdl_file = SDL_RWFromFile(path.raw_pointer_to_characters(), file_access_mode_strings[mode].raw_pointer_to_characters());
    result.isOpen = (result.sdl_file != nullptr);

    return result;
}

void closeFile(FileHandle* file)
{
    DEBUG_FUNCTION();

    if (file->isOpen) {
        SDL_RWclose(file->sdl_file);
        file->isOpen = false;
    }
}

s64 getFilePosition(FileHandle* file)
{
    s64 result = -1;

    if (file->isOpen) {
        result = SDL_RWtell(file->sdl_file);
    }

    return result;
}

smm getFileSize(FileHandle* file)
{
    smm fileSize = -1;

    if (file->isOpen) {
        s64 currentPos = SDL_RWtell(file->sdl_file);

        fileSize = (smm)SDL_RWseek(file->sdl_file, 0, RW_SEEK_END);
        SDL_RWseek(file->sdl_file, currentPos, RW_SEEK_SET);
    }

    return fileSize;
}

bool deleteFile(String path)
{
    ASSERT(path.is_null_terminated());
    return platform_deleteFile(path);
}

bool createDirectory(String path)
{
    ASSERT(path.is_null_terminated());
    return platform_createDirectory(path);
}

smm readFromFile(FileHandle* file, Blob& dest)
{
    DEBUG_FUNCTION();

    smm bytesRead = 0;

    if (file->isOpen) {
        bytesRead = SDL_RWread(file->sdl_file, dest.writable_data(), 1, dest.size());
    }

    return bytesRead;
}

smm readData(FileHandle* file, smm position, smm size, void* result)
{
    DEBUG_FUNCTION();

    smm bytesRead = 0;

    if (file->isOpen) {
        if (SDL_RWseek(file->sdl_file, position, RW_SEEK_SET) != -1) {
            bytesRead = SDL_RWread(file->sdl_file, result, 1, size);
        }
    }

    return bytesRead;
}

File readFile(FileHandle* handle, MemoryArena* arena)
{
    DEBUG_FUNCTION();

    File result = {};
    result.name = handle->path;
    result.isLoaded = false;

    if (handle->isOpen) {
        smm fileSize = getFileSize(handle);
        result.data = arena->allocate_blob(fileSize);
        smm bytesRead = readFromFile(handle, result.data);

        if (bytesRead != fileSize) {
            logWarn("File '{0}' was only partially read. Size {1}, read {2}"_s, { handle->path, formatInt(fileSize), formatInt(bytesRead) });
        } else {
            result.isLoaded = true;
        }
    } else {
        logWarn("Failed to open file '{0}' for reading."_s, { handle->path });
    }

    return result;
}

File readFile(MemoryArena* memoryArena, String filePath)
{
    DEBUG_FUNCTION();

    FileHandle handle = openFile(filePath, FileAccessMode::Read);
    File result = readFile(&handle, memoryArena);
    closeFile(&handle);

    return result;
}

bool writeFile(String filePath, String contents)
{
    DEBUG_FUNCTION();

    bool succeeded = false;

    FileHandle file = openFile(filePath, FileAccessMode::Write);

    if (file.isOpen) {
        smm bytesWritten = SDL_RWwrite(file.sdl_file, contents.raw_pointer_to_characters(), 1, contents.length());
        succeeded = (bytesWritten == contents.length());
        closeFile(&file);
    } else {
        logError("Failed to open file '{0}' for writing."_s, { filePath });
    }

    return succeeded;
}

bool writeToFile(FileHandle* file, smm dataLength, void* data)
{
    bool succeeded = false;

    if (file->isOpen) {
        smm bytesWritten = SDL_RWwrite(file->sdl_file, data, 1, dataLength);
        succeeded = (bytesWritten == dataLength);
    } else {
        logError("Cannot write to file '{0}'."_s, { file->path });
    }

    return succeeded;
}
