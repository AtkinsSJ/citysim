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

FileBlob appendBlob(s32 currentOffset, WriteBuffer *buffer, s32 length, u8 *data, FileBlobCompressionScheme scheme)
{
	FileBlob result = {};
	result.compressionScheme = scheme;
	result.relativeOffset = currentOffset;
	result.decompressedLength = length;

	switch (scheme)
	{
		case Blob_Uncompressed: {
			append(buffer, length, data);
			result.length = length;
		} break;

		case Blob_RLE_S8: {
			result.length = appendRLE(buffer, length, data);
		} break;

		default: {
			logError("Called appendBlob() with an unrecognized scheme! ({0}) Defaulting to Blob_Uncompressed."_s, {formatInt(scheme)});
			result = appendBlob(currentOffset, buffer, length, data, Blob_Uncompressed);
		} break;
	}

	return result;
}

FileBlob appendBlob(s32 currentOffset, WriteBuffer *buffer, Array2<u8> *data, FileBlobCompressionScheme scheme)
{
	return appendBlob(currentOffset, buffer, data->w * data->h, data->items, scheme);
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
	initWriteBuffer(&writer.buffer);

	FileHeader fileHeader = makeFileHeader(identifier, version);
	appendStruct<FileHeader>(&writer.buffer, &fileHeader);

	return writer;
}

void FileWriter::addTOCEntry(FileIdentifier sectionID)
{
	// TODO: Implement!

	// s32 startOffset = reserve(&buffer, sizeof(FileTOCEntry));

	// FileTOCEntry entry = {};

	// entry.sectionID = sectionID;

}

template <typename T>
T *FileWriter::startSection(FileIdentifier sectionID, u8 sectionVersion)
{
	this->startOfSectionHeader = reserve(&buffer, sizeof(FileSectionHeader));
	this->startOfSectionData = getCurrentPosition(&buffer);

	this->sectionHeader = {};
	this->sectionHeader.identifier = sectionID;
	this->sectionHeader.version = sectionVersion;

	return null;
}

void FileWriter::endSection()
{
	// TODO: Update TOC!

	this->sectionHeader.length = getCurrentPosition(&buffer) - this->startOfSectionData;
	overwriteAt(&buffer, startOfSectionHeader, sizeof(FileSectionHeader), &this->sectionHeader);
}

bool FileWriter::outputToFile(FileHandle *file)
{
	return writeToFile(file, &buffer);
}
