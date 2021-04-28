
BinaryFileReader readBinaryFile(FileHandle *handle, FileIdentifier identifier)
{
	BinaryFileReader reader = {};
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
			Maybe<Array<FileTOCEntry>> maybeToc = reader.readArray<FileTOCEntry>(0, header.toc);
			if (maybeToc.isValid)
			{
				reader.toc = maybeToc.value;
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
				currentSection = allocateBlob(tempArena, bytesToRead);
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

		if ((relativeOffset >= 0) && ((structStartPos + sizeof(T)) < currentSectionHeader->length))
		{
			result = (T*) (currentSection.memory + structStartPos);
		}
	}

	return result;
}

String BinaryFileReader::readString(FileString fileString)
{
	String result = nullString;

	if (isValidFile && currentSectionHeader != null)
	{
		smm stringStartPos = sizeof(FileSectionHeader) + fileString.relativeOffset;
		result = makeString((char *)(currentSection.memory + stringStartPos), fileString.length, false);
	}

	return result;
}

template <typename T>
Maybe<Array<T>> BinaryFileReader::readArray(smm basePosition, FileArray fileArray)
{
	Maybe<Array<T>> result = makeFailure<Array<T>>();

	Array<T> array = allocateArray<T>(tempArena, fileArray.count, true);

	smm byteLength = fileArray.count * sizeof(T);
	smm bytesRead = readData(fileHandle, basePosition + fileArray.relativeOffset, byteLength, array.items);
	if (bytesRead == byteLength)
	{
		result = makeSuccess(array);
	}

	return result;
}
