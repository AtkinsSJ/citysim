#pragma once

// Returns the part of 'filename' after the final '.'
// eg, getFileExtension("foo.bar.baz") would return "baz".
// If there is no '.', we return an empty String.
String getFileExtension(String filename)
{
    String fileExtension = {};

    s32 length = 0;
    while ((length < filename.length) && (filename[filename.length - length - 1] != '.')) {
        length++;
    }

    if (length == filename.length) {
        // no extension!
        length = 0;
    }

    fileExtension = makeString(filename.chars + filename.length - length, length);

    return fileExtension;
}

String getFileName(String filename)
{
    String result = filename;

    s32 length = 0;
    while ((length < filename.length) && (filename[filename.length - length - 1] != '.')) {
        length++;
    }

    if (length == filename.length) {
        // no extension!
    } else {
        result.length -= length + 1;
    }

    return result;
}

String constructPath(std::initializer_list<String> parts, bool appendWildcard)
{
    return platform_constructPath(parts, appendWildcard);
}

String getFileLocale(String filename)
{
    String result = {};

    Maybe<s32> endPos = findIndexOfChar(filename, '.', true);
    if (endPos.isValid) {
        Maybe<s32> startPos = findIndexOfChar(filename, '.', true, endPos.value - 1);
        if (startPos.isValid) {
            result = makeString(filename.chars + startPos.value + 1, endPos.value - startPos.value - 1);
        }
    }

    return result;
}

FileHandle openFile(String path, FileAccessMode mode)
{
    DEBUG_FUNCTION();

    ASSERT(isNullTerminated(path)); // openFile() path must be null-terminated.

    FileHandle result = {};

    result.path = path;
    result.sdl_file = SDL_RWFromFile(path.chars, fileAccessModeStrings[mode]);
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
    ASSERT(isNullTerminated(path));
    return platform_deleteFile(path);
}

bool createDirectory(String path)
{
    ASSERT(isNullTerminated(path));
    return platform_createDirectory(path);
}

smm readFromFile(FileHandle* file, smm size, u8* memory)
{
    DEBUG_FUNCTION();

    smm bytesRead = 0;

    if (file->isOpen) {
        bytesRead = SDL_RWread(file->sdl_file, memory, 1, size);
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

File readFile(FileHandle* handle, MemoryArena* arena)
{
    DEBUG_FUNCTION();

    File result = {};
    result.name = handle->path;
    result.isLoaded = false;

    if (handle->isOpen) {
        smm fileSize = getFileSize(handle);
        result.data = allocateBlob(arena, fileSize);
        smm bytesRead = readFromFile(handle, fileSize, result.data.memory);

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

    FileHandle handle = openFile(filePath, FileAccess_Read);
    File result = readFile(&handle, memoryArena);
    closeFile(&handle);

    return result;
}

bool writeFile(String filePath, String contents)
{
    DEBUG_FUNCTION();

    bool succeeded = false;

    FileHandle file = openFile(filePath, FileAccess_Write);

    if (file.isOpen) {
        smm bytesWritten = SDL_RWwrite(file.sdl_file, contents.chars, 1, contents.length);
        succeeded = (bytesWritten == contents.length);
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

template<typename T>
bool writeToFile(FileHandle* file, T* data)
{
    return writeToFile(file, sizeof(T), data);
}

DirectoryListingHandle beginDirectoryListing(String path, FileInfo* result)
{
    return platform_beginDirectoryListing(path, result);
}

bool nextFileInDirectory(DirectoryListingHandle* handle, FileInfo* result)
{
    bool foundAResult = false;
    if (handle->isValid) {
        foundAResult = platform_nextFileInDirectory(handle, result);
    }
    return foundAResult;
}

void stopDirectoryListing(DirectoryListingHandle* handle)
{
    if (handle->isValid) {
        platform_stopDirectoryListing(handle);
        handle->isValid = false;
    }
}

inline bool hasNextFile(iterateDirectoryListing* iterator)
{
    return iterator->handle.isValid;
}

inline void findNextFile(iterateDirectoryListing* iterator)
{
    nextFileInDirectory(&iterator->handle, &iterator->fileInfo);
}

inline FileInfo* getFileInfo(iterateDirectoryListing* iterator)
{
    return &iterator->fileInfo;
}

DirectoryChangeWatchingHandle beginWatchingDirectory(String path)
{
    return platform_beginWatchingDirectory(path);
}

bool hasDirectoryChanged(DirectoryChangeWatchingHandle* handle)
{
    bool result = false;
    if (handle->isValid) {
        result = platform_hasDirectoryChanged(handle);
    }
    return result;
}

void stopWatchingDirectory(DirectoryChangeWatchingHandle* handle)
{
    if (handle->isValid) {
        platform_stopWatchingDirectory(handle);
        handle->isValid = false;
    }
}
