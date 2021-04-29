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
		logError("Binary file '{0}' does not begin with the expected 4-byte sequence. Expected '{1}', got '{2}'"_s, {
			saveFileName,
			makeString((char*)(&identifier), 4),
			makeString((char*)(&fileHeader->identifier), 4)
		});
		isValid = false;
	}
	else if ((fileHeader->unixNewline != 0x0A)
		  || (fileHeader->dosNewline[0] != 0x0D) || (fileHeader->dosNewline[1] != 0x0A))
	{
		logError("Binary file '{0}' has corrupted newline characters. This probably means the saving or loading code is incorrect."_s, {
			saveFileName
		});
		isValid = false;
	}
	else if (fileHeader->version > BINARY_FILE_FORMAT_VERSION)
	{
		logError("Binary file '{0}' was created with a newer save file format than we understand. File version is '{1}', maximum we support is '{2}'"_s, {
			saveFileName,
			formatInt(fileHeader->version),
			formatInt(BINARY_FILE_FORMAT_VERSION),
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

void rleDecode(u8 *source, u8 *dest, smm destSize)
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

bool decodeBlob(FileBlob blob, u8 *baseMemory, u8 *dest, smm destSize)
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
