#pragma once

// TODO: @Size Maybe we keep a buffer the size of the largest section in the
// file, to reduce how much memory we have to allocate. Though, that assumes
// we only ever need one section in memory at a time.
struct BinaryFileReader
{
	// Data about the file itself
	FileHandle *fileHandle;
	bool isValidFile;
	u32 problems;

	// TOC
	Array<FileTOCEntry> toc;

	// Working data for the section we're currently operating on
	FileIdentifier currentSectionID;
	FileSectionHeader *currentSectionHeader;
	Blob currentSection;

// Methods

	bool startSection(FileIdentifier sectionID, u8 supportedSectionVersion);

	template <typename T>
	T *readStruct(smm relativeOffset);

	String readString(FileString fileString);

	// For cases we don't handle, you can directly access the bytes of the current section with this.
	// But generally, you want to use readX() methods instead.
	// TODO: Ideally, we don't want to allow this at all, but instead have a safe way of reading arbitrary data.
	u8 *startReadingBytes(smm relativeOffset);
};

enum BinaryFileProblems
{
	BFP_InvalidFormat   = 1 << 0,
	BFP_WrongIdentifier = 1 << 1,
	BFP_VersionTooNew   = 1 << 2,
	BFP_CorruptTOC      = 1 << 3,
};

BinaryFileReader readBinaryFile(FileHandle *handle, FileIdentifier identifier);
