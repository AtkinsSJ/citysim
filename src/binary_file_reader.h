/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "binary_file.h"
#include "file.h"
#include <Util/MemoryArena.h>

// TODO: @Size Maybe we keep a buffer the size of the largest section in the
// file, to reduce how much memory we have to allocate. Though, that assumes
// we only ever need one section in memory at a time.
struct BinaryFileReader {
    MemoryArena* arena;
    MemoryArenaResetState arenaResetState;

    // Data about the file itself
    FileHandle* fileHandle;
    bool isValidFile;
    u32 problems;

    // TOC
    Array<FileTOCEntry> toc;

    // Working data for the section we're currently operating on
    FileIdentifier currentSectionID;
    FileSectionHeader* currentSectionHeader;
    Blob currentSection;

    // Methods

    bool startSection(FileIdentifier sectionID, u8 supportedSectionVersion);

    template<typename T>
    T* readStruct(smm relativeOffset)
    {
        T* result = nullptr;

        if (isValidFile && currentSectionHeader != nullptr) {
            // Make sure that T actually fits where it was requested
            smm structStartPos = sizeof(FileSectionHeader) + relativeOffset;

            if ((relativeOffset >= 0) && ((relativeOffset + sizeof(T)) <= currentSectionHeader->length)) {
                result = (T*)(currentSection.memory + structStartPos);
            }
        }

        return result;
    }

    template<typename T>
    bool readArray(FileArray source, Array<T>* dest)
    {
        bool succeeded = false;

        if (source.count <= (u32)dest->capacity) {
            if (source.count < (u32)dest->capacity) {
                logWarn("Destination passed to readArray() is larger than needed. (Need {0}, got {1})"_s, { formatInt(source.count), formatInt(dest->capacity) });
            }

            copyMemory<T>((T*)sectionMemoryAt(source.relativeOffset), dest->items, source.count);
            dest->count = source.count;
            succeeded = true;
        }

        return succeeded;
    }

    String readString(FileString fileString);

    // readBlob() functions return whether it succeeded
    bool readBlob(FileBlob source, u8* dest, smm destSize);

    template<typename T>
    bool readBlob(FileBlob source, Array<T>* dest)
    {
        smm destSize = dest->capacity * sizeof(T);

        bool succeeded = readBlob(source, (u8*)dest->items, destSize);
        if (succeeded) {
            dest->count = dest->capacity;
        }

        return succeeded;
    }

    template<typename T>
    bool readBlob(FileBlob source, Array2<T>* dest)
    {
        smm destSize = dest->w * dest->h * sizeof(T);

        return readBlob(source, (u8*)dest->items, destSize);
    }

    // Internal
    u8* sectionMemoryAt(smm relativeOffset);
};

enum BinaryFileProblems {
    BFP_InvalidFormat = 1 << 0,
    BFP_WrongIdentifier = 1 << 1,
    BFP_VersionTooNew = 1 << 2,
    BFP_CorruptTOC = 1 << 3,
};

BinaryFileReader readBinaryFile(FileHandle* handle, FileIdentifier identifier, MemoryArena* arena);
