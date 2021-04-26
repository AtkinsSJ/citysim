#pragma once

FileHeader makeFileHeader(FileIdentifier identifier, u8 version)
{
	FileHeader result = {};

	result.identifier    = identifier;
	result.version       = version;
	result.unixNewline   = 0x0A;
	result.dosNewline[0] = 0x0D;
	result.dosNewline[1] = 0x0A;

	return result;
}

bool fileHeaderIsValid(FileHeader *fileHeader, String saveFileName, FileIdentifier identifier)
{
	bool isValid = true;

	if (fileHeader->identifier != identifier)
	{
		logError("Save file '{0}' does not begin with the expected 4-byte sequence. Expected '{1}', got '{2}'"_s, {
			saveFileName,
			makeString((char*)(&identifier), 4),
			makeString((char*)(&fileHeader->identifier), 4)
		});
		isValid = false;
	}
	else if ((fileHeader->unixNewline != 0x0A)
		  || (fileHeader->dosNewline[0] != 0x0D) || (fileHeader->dosNewline[1] != 0x0A))
	{
		logError("Save file '{0}' has corrupted newline characters. This probably means the saving or loading code is incorrect."_s, {
			saveFileName
		});
		isValid = false;
	}

	return isValid;
}

bool checkFileHeaderVersion(FileHeader *fileHeader, String saveFileName, u8 currentVersion)
{
	bool isValid = true;

	if (fileHeader->version > currentVersion)
	{
		logError("Save file '{0}' was created with a newer save file format than we understand. File version is '{1}', maximum we support is '{2}'"_s, {
			saveFileName,
			formatInt(fileHeader->version),
			formatInt(currentVersion),
		});
		isValid = false;
	}

	return isValid;
}

template <typename T>
bool readArray(FileArray source, u8 *baseMemory, Array<T> *dest)
{
	bool succeeded = true;

	if ((u32)dest->capacity < source.count)
	{
		logError("Unable to decode array because the destination is too small! (Need {0}, got {1})"_s, {formatInt(source.count), formatInt(dest->capacity)});
		succeeded = false;
	}
	else
	{
		copyMemory<T>((T*)(baseMemory + source.relativeOffset), dest->items, source.count);
		dest->count = source.count;
	}

	return succeeded;
}

String readString(FileString source, u8 *base)
{
	return makeString((char *)(base + source.relativeOffset), source.length, false);
}

void rleDecode(u8 *source, u8 *dest, s32 destSize)
{
	u8 *sourcePos = source;
	u8 *destPos = dest;
	u8 *destEnd = dest + destSize;

	while (destPos < destEnd)
	{
		s8 length = *((s8 *)sourcePos);
		sourcePos++;
		if (length < 0)
		{
			// Literals
			s8 literalCount = -length;
			copyMemory(sourcePos, destPos, literalCount);
			sourcePos += literalCount;
			destPos += literalCount;
		}
		else
		{
			// RLE
			u8 value = *sourcePos;
			sourcePos++;
			fillMemory(destPos, value, length);
			destPos += length;
		}
	}
}

bool decodeBlob(FileBlob blob, u8 *baseMemory, u8 *dest, s32 destSize)
{
	bool succeeded = true;

	if ((u32)destSize < blob.decompressedLength)
	{
		logError("Unable to decode save file data blob because the destination is too small! (Need {0}, got {1})"_s, {formatInt(blob.decompressedLength), formatInt(destSize)});
		succeeded = false;
	}
	else
	{
		switch (blob.compressionScheme)
		{
			case Blob_Uncompressed: {
				copyMemory(baseMemory + blob.relativeOffset, dest, blob.length);
			} break;

			case Blob_RLE_S8: {
				rleDecode(baseMemory + blob.relativeOffset, dest, destSize);
			} break;

			default: {
				logError("Unable to decode save file data blob because the encoding is unrecognized! ({0})"_s, {formatInt(blob.compressionScheme)});
				succeeded = false;
			} break;
		}
	}

	return succeeded;
}

bool decodeBlob(FileBlob blob, u8 *baseMemory, Array2<u8> *dest)
{
	return decodeBlob(blob, baseMemory, dest->items, dest->w * dest->h);
}

FileWriter startWritingFile(FileIdentifier identifier, u8 version)
{
	FileWriter writer = {};
	writer.buffer.init();

	writer.fileHeaderLoc = writer.buffer.reserveStruct<FileHeader>();
	FileHeader fileHeader = makeFileHeader(identifier, version);
	fileHeader.toc.count = 0;
	fileHeader.toc.relativeOffset = writer.buffer.getLengthSince(writer.fileHeaderLoc);

	writer.buffer.overwriteAt(writer.fileHeaderLoc, sizeof(FileHeader), &fileHeader);

	return writer;
}

void FileWriter::addTOCEntry(FileIdentifier sectionID)
{
	ASSERT(!tocComplete);

	// Make sure this entry doesn't exist already
	Indexed<FileTOCEntry> existingTocEntry = findTOCEntry(sectionID);
	ASSERT(existingTocEntry.index == -1); // Duplicate TOC entry!

	// Add a TOC entry with no location or length
	FileTOCEntry tocEntry = {};
	tocEntry.sectionID = sectionID;
	buffer.appendStruct<FileTOCEntry>(&tocEntry);

	// Find the TOC entry count and increment it
	WriteBufferLocation tocCountLoc = fileHeaderLoc + offsetof(FileHeader, toc.count);
	leU32 tocCount = buffer.readAt<leU32>(tocCountLoc);
	tocCount = tocCount + 1;
	buffer.overwriteAt(tocCountLoc, sizeof(leU32), &tocCount);
}

void FileWriter::startSection(FileIdentifier sectionID, u8 sectionVersion)
{
	tocComplete = true;
	
	startOfSectionHeader = buffer.reserveStruct<FileSectionHeader>();
	startOfSectionData = buffer.getCurrentPosition();

	sectionHeader = {};
	sectionHeader.identifier = sectionID;
	sectionHeader.version = sectionVersion;

	// Find our TOC entry
	Indexed<FileTOCEntry> tocEntry = findTOCEntry(sectionID);
	ASSERT(tocEntry.index >= 0); // Must add a TOC entry for each section in advance!
	sectionTOCLoc = tocEntry.index;
}

s32 FileWriter::getSectionRelativeOffset()
{
	return buffer.getCurrentPosition() - startOfSectionData;
}

template <typename T>
FileArray FileWriter::appendArray(Array<T> data)
{
	FileArray result = {};
	result.count = data.count;
	result.relativeOffset = getSectionRelativeOffset();

	for (s32 i=0; i < data.count; i++)
	{
		buffer.appendStruct<T>(&data[i]);
	}

	return result;
}

FileBlob FileWriter::appendBlob(s32 length, u8 *data, FileBlobCompressionScheme scheme)
{
	FileBlob result = {};
	result.compressionScheme = scheme;
	result.relativeOffset = getSectionRelativeOffset();
	result.decompressedLength = length;

	switch (scheme)
	{
		case Blob_Uncompressed: {
			buffer.appendBytes(length, data);
			result.length = length;
		} break;

		case Blob_RLE_S8: {
			WriteBufferRange range = buffer.appendRLEBytes(length, data);
			result.length = range.length;
		} break;

		default: {
			logError("Called appendBlob() with an unrecognized scheme! ({0}) Defaulting to Blob_Uncompressed."_s, {formatInt(scheme)});
			result = appendBlob(length, data, Blob_Uncompressed);
		} break;
	}

	return result;
}

FileBlob FileWriter::appendBlob(Array2<u8> *data, FileBlobCompressionScheme scheme)
{
	return appendBlob(data->w * data->h, data->items, scheme);
}
	
FileString FileWriter::appendString(String s)
{
	FileString result = {};

	result.length = s.length;
	result.relativeOffset = getSectionRelativeOffset();
	buffer.appendBytes(s.length, s.chars);

	return result;
}

void FileWriter::endSection()
{
	// Update section header
	u32 sectionLength = buffer.getLengthSince(startOfSectionData);
	sectionHeader.length = sectionLength;
	buffer.overwriteAt(startOfSectionHeader, sizeof(FileSectionHeader), &sectionHeader);

	// Update TOC
	FileTOCEntry tocEntry = buffer.readAt<FileTOCEntry>(sectionTOCLoc);
	tocEntry.offset = startOfSectionHeader;
	tocEntry.length = sectionLength;
	buffer.overwriteAt(sectionTOCLoc, sizeof(FileTOCEntry), &tocEntry);
}

bool FileWriter::outputToFile(FileHandle *file)
{
	return buffer.writeToFile(file);
}

Indexed<FileTOCEntry> FileWriter::findTOCEntry(FileIdentifier sectionID)
{
	Indexed<FileTOCEntry> result;
	result.index = -1; // Assume failure

	// Find our TOC entry
	// So, we have to walk the array, which is... interesting
	// TODO: maybe just keep a table of toc entry locations?
	FileHeader fileHeader = buffer.readAt<FileHeader>(fileHeaderLoc);
	WriteBufferLocation tocEntryLoc = fileHeaderLoc + fileHeader.toc.relativeOffset;
	for (u32 i=0; i < fileHeader.toc.count; i++)
	{
		FileTOCEntry tocEntry = buffer.readAt<FileTOCEntry>(tocEntryLoc);

		if (tocEntry.sectionID == sectionID)
		{
			result = makeIndexedValue<FileTOCEntry>(tocEntry, tocEntryLoc);
			break;
		}

		tocEntryLoc += sizeof(FileTOCEntry);
	}

	return result;
}