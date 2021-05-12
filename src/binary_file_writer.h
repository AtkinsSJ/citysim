#pragma once

struct BinaryFileWriter
{
	MemoryArena *arena;

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

	template <typename T>
	void startSection(FileIdentifier sectionID, u8 sectionVersion);

	s32 getSectionRelativeOffset();

	template <typename T>
	FileArray appendArray(Array<T> data);

	template <typename T>
	WriteBufferRange reserveArray(s32 length);
	template <typename T>
	FileArray writeArray(Array<T> data, WriteBufferRange location);

	FileBlob appendBlob(s32 length, u8 *data, FileBlobCompressionScheme scheme);
	FileBlob appendBlob(Array2<u8> *data, FileBlobCompressionScheme scheme);
	
	FileString appendString(String s);

	template <typename T>
	void endSection(T *sectionStruct);

	bool outputToFile(FileHandle *file);

// Internal
	Maybe<WriteBufferRange> findTOCEntry(FileIdentifier sectionID);
};

BinaryFileWriter startWritingFile(FileIdentifier identifier, u8 version, MemoryArena *arena);
