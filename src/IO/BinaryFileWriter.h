/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/BinaryFile.h>
#include <IO/File.h>
#include <IO/WriteBuffer.h>
#include <Util/MemoryArena.h>

struct BinaryFileWriter {
    MemoryArena* arena;

    WriteBuffer buffer;
    WriteBufferRange fileHeaderLoc;

    // TODO: Maybe keep the TOC in a more convenient format, like a HashTable? If
    // so, we'd probably want to decide in advance how many TOC entries there can
    // be, in startWritingFile(), and we can then get rid of addTOCEntry() and
    // just do the TOC adding in startSection(). But that's all feeling too
    // complicated for me today.
    bool tocComplete;

    FileSectionHeader sectionHeader;
    WriteBufferRange sectionTOCRange;
    WriteBufferRange sectionHeaderRange;
    WriteBufferLocation startOfSectionData;

    // Methods

    void addTOCEntry(FileIdentifier sectionID);

    template<typename T>
    void startSection(FileIdentifier sectionID, u8 sectionVersion)
    {
        tocComplete = true;

        sectionHeaderRange = buffer.reserve<FileSectionHeader>();
        startOfSectionData = buffer.getCurrentPosition();

        sectionHeader = {};
        sectionHeader.identifier = sectionID;
        sectionHeader.version = sectionVersion;

        // Find our TOC entry
        Maybe<WriteBufferRange> tocEntry = findTOCEntry(sectionID);
        ASSERT(tocEntry.isValid); // Must add a TOC entry for each section in advance!
        sectionTOCRange = tocEntry.value;

        // Reserve our "section struct"
        buffer.reserve<T>();
    }

    s32 getSectionRelativeOffset();

    template<typename T>
    FileArray appendArray(Array<T> data)
    {
        FileArray result = {};
        result.count = data.count;
        result.relativeOffset = getSectionRelativeOffset();

        for (s32 i = 0; i < data.count; i++) {
            buffer.append<T>(&data[i]);
        }

        return result;
    }

    template<typename T>
    WriteBufferRange reserveArray(s32 length)
    {
        return buffer.reserveBytes(length * sizeof(T));
    }

    template<typename T>
    FileArray writeArray(Array<T> data, WriteBufferRange location)
    {
        s32 dataLength = data.count * sizeof(T);
        ASSERT(dataLength <= location.length);

        buffer.overwriteAt(location.start, dataLength, data.items);

        FileArray result = {};
        result.count = data.count;
        result.relativeOffset = location.start - startOfSectionData;
        return result;
    }

    FileBlob appendBlob(s32 length, u8* data, FileBlobCompressionScheme scheme);
    FileBlob appendBlob(Array2<u8>* data, FileBlobCompressionScheme scheme);
    template<Enum EnumT>
    FileBlob appendBlob(Array2<EnumT> const& data, FileBlobCompressionScheme scheme)
    {
        // FIXME: Allow non-u8. Theoretically it'll work right now, but our compression scheme goes by individual bytes
        //        so we should do something smarter for larger types.
        static_assert(sizeof(EnumT) == 1);
        return appendBlob(data.w * data.h, reinterpret_cast<EnumUnderlyingType<EnumT>*>(data.items), scheme);
    }

    FileString appendString(String s);

    template<typename T>
    void endSection(T* sectionStruct)
    {
        buffer.overwriteAt(startOfSectionData, sizeof(T), sectionStruct);

        // Update section header
        u32 sectionLength = buffer.getLengthSince(startOfSectionData);
        sectionHeader.length = sectionLength;
        buffer.overwriteAt<FileSectionHeader>(sectionHeaderRange, &sectionHeader);

        // Update TOC
        FileTOCEntry tocEntry = buffer.readAt<FileTOCEntry>(sectionTOCRange);
        tocEntry.offset = sectionHeaderRange.start;
        tocEntry.length = sectionLength;
        buffer.overwriteAt<FileTOCEntry>(sectionTOCRange, &tocEntry);
    }

    bool outputToFile(FileHandle* file);

    // Internal
    Maybe<WriteBufferRange> findTOCEntry(FileIdentifier sectionID);
    s8 countRunLength(s32 dataLength, u8* data);
};

BinaryFileWriter startWritingFile(FileIdentifier identifier, u8 version, MemoryArena* arena);
