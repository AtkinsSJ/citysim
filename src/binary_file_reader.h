#pragma once

// TODO: @Size Maybe we keep a buffer the size of the largest section in the
// file, to reduce how much memory we have to allocate. Though, that assumes
// we only ever need one section in memory at a time.
struct BinaryFileReader
{
	FileHandle *fileHandle;
	bool isValidFile;
	u32 problems;

	Array<FileTOCEntry> toc;

	FileIdentifier currentSectionID;
	FileSectionHeader *currentSectionHeader;
	Blob currentSection;

// Methods

	bool startSection(FileIdentifier sectionID, u8 supportedSectionVersion);

	template <typename T>
	T *readStruct(smm relativeOffset);

	String readString(FileString fileString);

	template <typename T>
	Maybe<Array<T>> readArray(smm basePosition, FileArray fileArray);
};

enum BinaryFileProblems
{
	BFP_InvalidFormat   = 1 << 0,
	BFP_WrongIdentifier = 1 << 1,
	BFP_VersionTooNew   = 1 << 2,
	BFP_CorruptTOC      = 1 << 3,
};

BinaryFileReader readBinaryFile(FileHandle *handle, FileIdentifier identifier);
