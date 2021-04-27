#pragma once

struct BinaryFileWriter
{
	WriteBuffer buffer;
	WriteBufferLocation fileHeaderLoc;
	bool tocComplete;

	FileSectionHeader sectionHeader;
	WriteBufferLocation sectionTOCLoc;
	WriteBufferLocation startOfSectionHeader;
	WriteBufferLocation startOfSectionData;

// Methods

	void addTOCEntry(FileIdentifier sectionID);

	void startSection(FileIdentifier sectionID, u8 sectionVersion);

	s32 getSectionRelativeOffset();

	template <typename T>
	FileArray appendArray(Array<T> data);

	FileBlob appendBlob(s32 length, u8 *data, FileBlobCompressionScheme scheme);
	FileBlob appendBlob(Array2<u8> *data, FileBlobCompressionScheme scheme);

	// Relative to current section
	FileString appendString(String s);

	void endSection();

	bool outputToFile(FileHandle *file);

// Internal
	Indexed<FileTOCEntry> findTOCEntry(FileIdentifier sectionID);
};

BinaryFileWriter startWritingFile(FileIdentifier identifier, u8 version);
