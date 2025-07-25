/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "binary_file_writer.h"
#include <Util/Enum.h>
#include <Util/Log.h>

BinaryFileWriter startWritingFile(FileIdentifier identifier, u8 version, MemoryArena* arena)
{
    BinaryFileWriter writer = {};
    writer.arena = arena;
    writer.buffer.init(KB(4), arena);

    writer.fileHeaderLoc = writer.buffer.reserve<FileHeader>();

    FileHeader fileHeader = {};
    fileHeader.identifier = identifier;
    fileHeader.version = version;
    fileHeader.unixNewline = 0x0A;
    fileHeader.dosNewline[0] = 0x0D;
    fileHeader.dosNewline[1] = 0x0A;
    fileHeader.toc.count = 0;
    fileHeader.toc.relativeOffset = writer.buffer.getLengthSince(writer.fileHeaderLoc.start);

    writer.buffer.overwriteAt<FileHeader>(writer.fileHeaderLoc, &fileHeader);

    return writer;
}

void BinaryFileWriter::addTOCEntry(FileIdentifier sectionID)
{
    ASSERT(!tocComplete);

    // Make sure this entry doesn't exist already
    Maybe<WriteBufferRange> existingTocEntry = findTOCEntry(sectionID);
    ASSERT(!existingTocEntry.isValid); // Duplicate TOC entry!

    // Add a TOC entry with no location or length
    FileTOCEntry tocEntry = {};
    tocEntry.sectionID = sectionID;
    buffer.append<FileTOCEntry>(&tocEntry);

    // Find the TOC entry count and increment it
    WriteBufferLocation tocCountLoc = fileHeaderLoc.start + offsetof(FileHeader, toc.count);
    leU32 tocCount = buffer.readAt<leU32>(tocCountLoc);
    tocCount = tocCount + 1;
    buffer.overwriteAt(tocCountLoc, sizeof(leU32), &tocCount);
}

s32 BinaryFileWriter::getSectionRelativeOffset()
{
    return buffer.getCurrentPosition() - startOfSectionData;
}

FileBlob BinaryFileWriter::appendBlob(s32 length, u8* data, FileBlobCompressionScheme scheme)
{
    FileBlob result = {};
    result.compressionScheme = to_underlying(scheme);
    result.relativeOffset = getSectionRelativeOffset();
    result.decompressedLength = length;

    switch (scheme) {
    case FileBlobCompressionScheme::Uncompressed: {
        buffer.appendBytes(length, data);
        result.length = length;
    } break;

    case FileBlobCompressionScheme::RLE_S8: {

        // Our scheme is, (s8 length, u8...data)
        // Positive length = repeat the next byte `length` times.
        // Negative length = copy the next `-length` bytes literally.

        s32 const minRunLength = 4; // This is a fairly arbitrary number! Maybe it should be bigger, idk.

        u8* end = data + length;
        u8* pos = data;
        s32 remainingLength = length;

        while (pos < end) {
            // Detect if this is a run
            s8 runLength = countRunLength(min(remainingLength, s8Max), pos);
            if (runLength >= minRunLength) {
                // Output the run!
                buffer.append<s8>(&runLength);
                buffer.append<u8>(pos);

                pos += runLength;
                remainingLength -= runLength;
            } else {
                // This is literals!
                s32 literalLength = 0;

                // We detect the end of the literals by repeatedly testing if the next bytes are a run
                runLength = countRunLength(min(remainingLength, s8Max), pos + literalLength);
                while ((runLength < minRunLength)
                    && (remainingLength > 0)) {
                    literalLength += runLength;
                    remainingLength -= runLength;
                    runLength = countRunLength(min(remainingLength, s8Max), pos + literalLength);
                }

                // Output literals
                s8 outputLength = truncate<s8>(-literalLength);
                buffer.append<s8>(&outputLength);
                buffer.appendBytes(literalLength, pos);
                pos += literalLength;

                // Also, if we did detect a run, output that too!
                if (runLength >= minRunLength) {
                    buffer.append<s8>(&runLength);
                    buffer.append<u8>(pos);

                    pos += runLength;
                    remainingLength -= runLength;
                }
            }
        }

        result.length = getSectionRelativeOffset() - result.relativeOffset;
    } break;

    default: {
        logError("Called appendBlob() with an unrecognized scheme! ({0}) Defaulting to FileBlobCompressionScheme::Uncompressed."_s, { formatInt(scheme) });
        result = appendBlob(length, data, FileBlobCompressionScheme::Uncompressed);
    } break;
    }

    return result;
}

FileBlob BinaryFileWriter::appendBlob(Array2<u8>* data, FileBlobCompressionScheme scheme)
{
    return appendBlob(data->w * data->h, data->items, scheme);
}

FileString BinaryFileWriter::appendString(String s)
{
    FileString result = {};

    result.length = s.length;
    result.relativeOffset = getSectionRelativeOffset();
    buffer.appendBytes(s.length, s.chars);

    return result;
}

bool BinaryFileWriter::outputToFile(FileHandle* file)
{
    // Check that the TOC entries are all filled-in

    // TODO: Maybe in release, we just remove the empty TOC entries? That'll
    // leave a gap in the file, but as long as we move later TOC entries so that
    // they're contiguous, it should be fine!
    FileHeader fileHeader = buffer.readAt<FileHeader>(fileHeaderLoc);
    WriteBufferLocation tocEntryLoc = fileHeaderLoc.start + fileHeader.toc.relativeOffset;
    for (u32 i = 0; i < fileHeader.toc.count; i++) {
        FileTOCEntry tocEntry = buffer.readAt<FileTOCEntry>(tocEntryLoc);

        ASSERT(tocEntry.offset != 0 || tocEntry.length != 0);

        tocEntryLoc += sizeof(FileTOCEntry);
    }

    return buffer.writeToFile(file);
}

Maybe<WriteBufferRange> BinaryFileWriter::findTOCEntry(FileIdentifier sectionID)
{
    Maybe<WriteBufferRange> result = makeFailure<WriteBufferRange>();

    // Find our TOC entry
    // So, we have to walk the array, which is... interesting
    // TODO: maybe just keep a table of toc entry locations?
    FileHeader fileHeader = buffer.readAt<FileHeader>(fileHeaderLoc);

    WriteBufferRange tocEntryRange = {};
    tocEntryRange.start = fileHeaderLoc.start + fileHeader.toc.relativeOffset;
    tocEntryRange.length = sizeof(FileTOCEntry);

    for (u32 i = 0; i < fileHeader.toc.count; i++) {
        FileTOCEntry tocEntry = buffer.readAt<FileTOCEntry>(tocEntryRange);

        if (tocEntry.sectionID == sectionID) {
            result = makeSuccess(tocEntryRange);
            break;
        }

        tocEntryRange.start += sizeof(FileTOCEntry);
    }

    return result;
}

s8 BinaryFileWriter::countRunLength(s32 dataLength, u8* data)
{
    s8 runLength = 1;

    while ((runLength < dataLength)
        && (runLength < s8Max)
        && (data[runLength] == data[0])) {
        runLength++;
    }

    return runLength;
}
