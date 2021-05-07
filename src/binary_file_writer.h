#pragma once

struct BinaryFileWriter
{
	MemoryArena *arena;

	WriteBuffer buffer;
	WriteBufferLocation fileHeaderLoc;
	bool tocComplete;

	FileSectionHeader sectionHeader;
	WriteBufferLocation sectionTOCLoc;
	WriteBufferLocation startOfSectionHeader;
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
	Indexed<FileTOCEntry> findTOCEntry(FileIdentifier sectionID);
};

BinaryFileWriter startWritingFile(FileIdentifier identifier, u8 version, MemoryArena *arena);
