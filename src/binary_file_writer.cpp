

BinaryFileWriter startWritingFile(FileIdentifier identifier, u8 version, MemoryArena *arena)
{
	BinaryFileWriter writer = {};
	writer.arena = arena;
	writer.buffer.init(KB(4), arena);

	writer.fileHeaderLoc = writer.buffer.reserveStruct<FileHeader>();

	FileHeader fileHeader = {};
	fileHeader.identifier    = identifier;
	fileHeader.version       = version;
	fileHeader.unixNewline   = 0x0A;
	fileHeader.dosNewline[0] = 0x0D;
	fileHeader.dosNewline[1] = 0x0A;
	fileHeader.toc.count = 0;
	fileHeader.toc.relativeOffset = writer.buffer.getLengthSince(writer.fileHeaderLoc);

	writer.buffer.overwriteAt(writer.fileHeaderLoc, sizeof(FileHeader), &fileHeader);

	return writer;
}

void BinaryFileWriter::addTOCEntry(FileIdentifier sectionID)
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

void BinaryFileWriter::startSection(FileIdentifier sectionID, u8 sectionVersion)
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

s32 BinaryFileWriter::getSectionRelativeOffset()
{
	return buffer.getCurrentPosition() - startOfSectionData;
}

template <typename T>
FileArray BinaryFileWriter::appendArray(Array<T> data)
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

FileBlob BinaryFileWriter::appendBlob(s32 length, u8 *data, FileBlobCompressionScheme scheme)
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

FileBlob BinaryFileWriter::appendBlob(Array2<u8> *data, FileBlobCompressionScheme scheme)
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

void BinaryFileWriter::endSection()
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

bool BinaryFileWriter::outputToFile(FileHandle *file)
{
	// Check that the TOC entries are all filled-in

	// TODO: Maybe in release, we just remove the empty TOC entries? That'll
	// leave a gap in the file, but as long as we move later TOC entries so that
	// they're contiguous, it should be fine!
	FileHeader fileHeader = buffer.readAt<FileHeader>(fileHeaderLoc);
	WriteBufferLocation tocEntryLoc = fileHeaderLoc + fileHeader.toc.relativeOffset;
	for (u32 i=0; i < fileHeader.toc.count; i++)
	{
		FileTOCEntry tocEntry = buffer.readAt<FileTOCEntry>(tocEntryLoc);

		ASSERT(tocEntry.offset != 0 || tocEntry.length != 0);

		tocEntryLoc += sizeof(FileTOCEntry);
	}

	return buffer.writeToFile(file);
}

Indexed<FileTOCEntry> BinaryFileWriter::findTOCEntry(FileIdentifier sectionID)
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
