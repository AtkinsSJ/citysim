
BinaryFileReader readBinaryFile(FileHandle *handle, FileIdentifier identifier, MemoryArena *arena)
{
	BinaryFileReader reader = {};
	reader.arena = arena;
	reader.fileHandle = handle;
	reader.isValidFile = false;
	reader.problems = 0;

	// Check this file is our format!
	FileHeader header;
	bool readHeader = readStruct<FileHeader>(handle, 0, &header);
	if (readHeader)
	{
		// Check for problems
		if ((header.unixNewline != 0x0A)
			  || (header.dosNewline[0] != 0x0D) || (header.dosNewline[1] != 0x0A))
		{
			logError("Binary file '{0}' has corrupted newline characters. This probably means the saving or loading code is incorrect."_s, {
				handle->path
			});
			reader.problems |= BFP_InvalidFormat;
		}
		else if (header.identifier != identifier)
		{
			logError("Binary file '{0}' does not begin with the expected 4-byte sequence. Expected '{1}', got '{2}'"_s, {
				handle->path,
				makeString((char*)(&identifier), 4),
				makeString((char*)(&header.identifier), 4)
			});
			reader.problems |= BFP_WrongIdentifier;
		}
		else if (header.version > BINARY_FILE_FORMAT_VERSION)
		{
			logError("Binary file '{0}' was created with a newer file format than we understand. File version is '{1}', maximum we support is '{2}'"_s, {
				handle->path,
				formatInt(header.version),
				formatInt(BINARY_FILE_FORMAT_VERSION),
			});
			reader.problems |= BFP_VersionTooNew;
		}
		else
		{
			// Read the TOC in
			Array<FileTOCEntry> toc = allocateArray<FileTOCEntry>(arena, header.toc.count, false);
			if (readArray(handle, header.toc.relativeOffset, header.toc.count, &toc))
			{
				reader.toc = toc;
				reader.isValidFile = true;
			}
			else
			{
				reader.problems |= BFP_CorruptTOC;
			}
		}
	}

	return reader;
}

bool BinaryFileReader::startSection(FileIdentifier sectionID, u8 supportedSectionVersion)
{
	bool succeeded = false;

	if (isValidFile)
	{
		if (sectionID != currentSectionID)
		{
			// Find the section in the TOC
			FileTOCEntry *tocEntry = null;
			for (s32 tocIndex = 0; tocIndex < toc.count; tocIndex++)
			{
				if (toc[tocIndex].sectionID == sectionID)
				{
					tocEntry = &toc[tocIndex];
					break;
				}
			}

			if (tocEntry != null)
			{
				// Read the whole section into memory
				smm bytesToRead = sizeof(FileSectionHeader) + tocEntry->length;
				currentSection = allocateBlob(arena, bytesToRead);
				smm bytesRead = readData(fileHandle, tocEntry->offset, bytesToRead, currentSection.memory);
				if (bytesRead == bytesToRead)
				{
					currentSectionHeader = (FileSectionHeader *)currentSection.memory;

					// Check the version
					if (currentSectionHeader->version <= supportedSectionVersion)
					{
						currentSectionID = sectionID;
						succeeded = true;
					}
					else
					{
						logError("Section '{0}' in file '{1}' is a higher version than we support!"_s, {toString(sectionID), fileHandle->path});
					}
				}
			}
			else
			{
				logError("Section '{0}' not found in TOC of '{1}'"_s, {toString(sectionID), fileHandle->path});
			}
		}
		else
		{
			succeeded = true;
		}
	}

	return succeeded;
}

template <typename T>
T *BinaryFileReader::readStruct(smm relativeOffset)
{
	T *result = null;

	if (isValidFile && currentSectionHeader != null)
	{
		// Make sure that T actually fits where it was requested
		smm structStartPos = sizeof(FileSectionHeader) + relativeOffset;

		if ((relativeOffset >= 0) && ((relativeOffset + sizeof(T)) <= currentSectionHeader->length))
		{
			result = (T*) (currentSection.memory + structStartPos);
		}
	}

	return result;
}

template <typename T>
bool BinaryFileReader::readArray(FileArray source, Array<T> *dest)
{
	bool succeeded = false;

	if (source.count <= (u32)dest->capacity)
	{
		if (source.count < (u32)dest->capacity)
		{
			logWarn("Destination passed to readArray() is larger than needed. (Need {0}, got {1})"_s, {formatInt(source.count), formatInt(dest->capacity)});
		}

		copyMemory<T>((T*)sectionMemoryAt(source.relativeOffset), dest->items, source.count);
		dest->count = source.count;
		succeeded = true;
	}

	return succeeded;
}

String BinaryFileReader::readString(FileString fileString)
{
	String result = nullString;

	if (isValidFile && currentSectionHeader != null)
	{
		result = makeString((char *)sectionMemoryAt(fileString.relativeOffset), fileString.length, false);
	}

	return result;
}

bool BinaryFileReader::readBlob(FileBlob source, u8 *dest, smm destSize)
{
	bool succeeded = false;

	if (source.decompressedLength <= destSize)
	{
		if (source.decompressedLength < destSize)
		{
			logWarn("Destination passed to readBlob() is larger than needed. (Need {0}, got {1})"_s, {formatInt(source.decompressedLength), formatInt(destSize)});
		}

		switch (source.compressionScheme)
		{
			case Blob_Uncompressed: {
				copyMemory(sectionMemoryAt(source.relativeOffset), dest, source.length);
				succeeded = true;
			} break;

			case Blob_RLE_S8: {
				rleDecode(sectionMemoryAt(source.relativeOffset), dest, destSize);
				succeeded = true;
			} break;

			default: {
				logError("Failed to decode data blob from file: unrecognised encoding scheme! ({0})"_s, {formatInt(source.compressionScheme)});
			} break;
		}
	}
	else
	{
		logError("Destination passed to readBlob() is too small! (Need {0}, got {1})"_s, {formatInt(source.decompressedLength), formatInt(destSize)});
	}

	return succeeded;
}

template <typename T>
bool BinaryFileReader::readBlob(FileBlob source, Array<T> *dest)
{
	smm destSize = dest->capacity * sizeof(T);

	bool succeeded = readBlob(source, (u8*)dest->items, destSize);
	if (succeeded)
	{
		dest->count = dest->capacity;
	}

	return succeeded;
}

template <typename T>
bool BinaryFileReader::readBlob(FileBlob source, Array2<T> *dest)
{
	smm destSize = dest->w * dest->h * sizeof(T);

	return readBlob(source, (u8*)dest->items, destSize);
}

u8 *BinaryFileReader::sectionMemoryAt(smm relativeOffset)
{
	return currentSection.memory + sizeof(FileSectionHeader) + relativeOffset;
}
