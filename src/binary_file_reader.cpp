/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "binary_file_reader.h"
#include <Util/Log.h>

BinaryFileReader readBinaryFile(FileHandle* handle, FileIdentifier identifier, MemoryArena* arena)
{
    BinaryFileReader reader = {};
    reader.arena = arena;
    reader.fileHandle = handle;
    reader.isValidFile = false;
    reader.problems = 0;

    // Check this file is our format!
    FileHeader header;
    bool readHeader = readStruct<FileHeader>(handle, 0, &header);
    if (readHeader) {
        // Check for problems
        if ((header.unixNewline != 0x0A)
            || (header.dosNewline[0] != 0x0D) || (header.dosNewline[1] != 0x0A)) {
            logError("Binary file '{0}' has corrupted newline characters. This probably means the saving or loading code is incorrect."_s, { handle->path });
            reader.problems |= BFP_InvalidFormat;
        } else if (header.identifier != identifier) {
            logError("Binary file '{0}' does not begin with the expected 4-byte sequence. Expected '{1}', got '{2}'"_s, { handle->path, makeString((char*)(&identifier), 4), makeString((char*)(&header.identifier), 4) });
            reader.problems |= BFP_WrongIdentifier;
        } else if (header.version > BINARY_FILE_FORMAT_VERSION) {
            logError("Binary file '{0}' was created with a newer file format than we understand. File version is '{1}', maximum we support is '{2}'"_s, {
                                                                                                                                                            handle->path,
                                                                                                                                                            formatInt(header.version),
                                                                                                                                                            formatInt(BINARY_FILE_FORMAT_VERSION),
                                                                                                                                                        });
            reader.problems |= BFP_VersionTooNew;
        } else {
            // Read the TOC in
            Array<FileTOCEntry> toc = arena->allocate_array<FileTOCEntry>(header.toc.count, false);
            if (readArray(handle, header.toc.relativeOffset, header.toc.count, &toc)) {
                reader.toc = toc;
                reader.isValidFile = true;
            } else {
                reader.problems |= BFP_CorruptTOC;
            }
        }
    }

    // Save the current arena state so we can revert to it after each section
    reader.arenaResetState = arena->get_current_position();

    return reader;
}

bool BinaryFileReader::startSection(FileIdentifier sectionID, u8 supportedSectionVersion)
{
    bool succeeded = false;

    if (isValidFile) {
        if (sectionID != currentSectionID) {
            arena->revert_to(arenaResetState);

            // Find the section in the TOC
            FileTOCEntry* tocEntry = nullptr;
            for (s32 tocIndex = 0; tocIndex < toc.count; tocIndex++) {
                if (toc[tocIndex].sectionID == sectionID) {
                    tocEntry = &toc[tocIndex];
                    break;
                }
            }

            if (tocEntry != nullptr) {
                // Read the whole section into memory
                smm bytesToRead = sizeof(FileSectionHeader) + tocEntry->length;
                currentSection = arena->allocate_blob(bytesToRead);
                smm bytesRead = readData(fileHandle, tocEntry->offset, bytesToRead, currentSection.writable_data());
                if (bytesRead == bytesToRead) {
                    currentSectionHeader = (FileSectionHeader*)currentSection.data();

                    // Check the version
                    if (currentSectionHeader->version <= supportedSectionVersion) {
                        currentSectionID = sectionID;
                        succeeded = true;
                    } else {
                        logError("Section '{0}' in file '{1}' is a higher version than we support!"_s, { toString(sectionID), fileHandle->path });
                    }
                }
            } else {
                logError("Section '{0}' not found in TOC of '{1}'"_s, { toString(sectionID), fileHandle->path });
            }
        } else {
            succeeded = true;
        }
    }

    return succeeded;
}

String BinaryFileReader::readString(FileString fileString)
{
    String result = nullString;

    if (isValidFile && currentSectionHeader != nullptr) {
        result = makeString((char*)sectionMemoryAt(fileString.relativeOffset), fileString.length, false);
    }

    return result;
}

bool BinaryFileReader::readBlob(FileBlob source, u8* dest, smm destSize)
{
    bool succeeded = false;

    if (source.decompressedLength <= destSize) {
        if (source.decompressedLength < destSize) {
            logWarn("Destination passed to readBlob() is larger than needed. (Need {0}, got {1})"_s, { formatInt(source.decompressedLength), formatInt(destSize) });
        }

        switch (source.compressionScheme) {
        case Blob_Uncompressed: {
            copyMemory(sectionMemoryAt(source.relativeOffset), dest, source.length);
            succeeded = true;
        } break;

        case Blob_RLE_S8: {
            rleDecode(sectionMemoryAt(source.relativeOffset), dest, destSize);
            succeeded = true;
        } break;

        default: {
            logError("Failed to decode data blob from file: unrecognised encoding scheme! ({0})"_s, { formatInt(source.compressionScheme) });
        } break;
        }
    } else {
        logError("Destination passed to readBlob() is too small! (Need {0}, got {1})"_s, { formatInt(source.decompressedLength), formatInt(destSize) });
    }

    return succeeded;
}

u8* BinaryFileReader::sectionMemoryAt(smm relativeOffset)
{
    return currentSection.writable_data() + sizeof(FileSectionHeader) + relativeOffset;
}
