#pragma once

/*

	It's our own binary file-format!

	It's basically a container format with individually-versioned sections, and
	attempts made to accomodate changes to the data without the format itself
	having to change.

	Files begin with a FileHeader, which tells you what kind of file it is, its
	version, where the table of contents can be found, and some fixed bytes used
	to detect if the file has been corrupted by newline conversion. The version
	refers to the overall container format - individual sections have their own
	versioning so changes can be made without invalidating the entire file.

	The table of contents is a series of FileTOCEntry's, which tell you where to
	find each section. This is to allow partial reading of the file if there's
	only one section you are interested in.

	The sections themselves each begin with a FileSectionHeader, which tells you
	the name, size and version of the section that follows directly after. Then,
	the section data can behave however it likes!

	For large blobs of data, FileBlob can be used. I wanted to avoid nailing down
	how each blob is stored, so FileBlob records the encoding scheme used,
	meaning that any blob can use any scheme and it will still be read correctly.
	(Well, assuming the game knows about that scheme, but that's what versioning
	is for!)

	FileTOCEntry and FileSectionHeader are a little redundant: We could eliminate
	the header and just put everything in the TOC entry. But, I like the apparent
	safety of having both. You know what to expect when following a TOC entry,
	and if you don't find a matching section header, you know something is wrong
	immediately. I think the minor increase in file size and computation time is
	worth it.

	- Sam, 21/04/2021
	
 */

#pragma pack(push, 1)

typedef u32 FileIdentifier;
inline FileIdentifier operator"" _id(const char *chars, size_t length)
{
	ASSERT(length == 4);
	return *((u32*)chars);
}

struct FileArray
{
	leU32 count; // Element count, not byte count
	leU32 relativeOffset;
};

struct FileBlob
{
	leU32 length;
	leU32 decompressedLength;
	leU32 relativeOffset;
	leU32 compressionScheme; // FileBlobCompressionScheme
};

enum FileBlobCompressionScheme
{
	Blob_Uncompressed = 0,
	Blob_RLE_S8 = 1, // Negative numbers are literal lengths, positive are run lengths

	//
	// TODO: Several per-tile-data arrays here are stored as u8s, but they only have a small number
	// of possible values. eg, zones are 0-3, or will be 0-10 if we add l/m/h densities. We could
	// fit two of those into a single byte, instead of 1 byte each. For terrain we only have 0-2
	// currently which would allow 4 tiles per byte! The downside is extra complexity, but also
	// it would make it harder to compress the resulting data. RLE works better when there are fewer
	// options, and eg for that terrain example, we'd going from 3 options to 81 options. Right now
	// the terrain size is reduced from 16000 to under 600, which is around 25%... which is the same
	// as the 4-tiles-per-byte system, but more complicated. So yeah, I don't know. To get the most
	// out of it we'd want an extra RLE or other compression scheme, that works with partial bytes.
	//
	// - Sam, 12/11/2019
	//
};

struct FileString
{
	leU32 length;
	leU32 relativeOffset;
};

struct FileHeader
{
	LittleEndian<FileIdentifier> identifier;
	leU8 version;

	// Bytes for checking for unwanted newline-conversion
	leU8 unixNewline; // = 0x0A;
	leU8 dosNewline[2]; // = {0x0D, 0x0A};

	// Offset within the file
	FileArray toc; // FileTOCEntry
};

struct FileTOCEntry
{
	FileIdentifier sectionID;
	leU32 offset; // Within file
	leU32 length;
};

struct FileSectionHeader
{
	LittleEndian<FileIdentifier> identifier;
	leU8 version;
	leU8 _pad[3]; // For future use maybe? Mostly just to make this be a multiple of 4 bytes.
	leU32 length; // Length of the chunk, NOT including the size of this FileSectionHeader
};

#pragma pack(pop)

FileHeader makeFileHeader(FileIdentifier identifier, u8 version);
bool fileHeaderIsValid(FileHeader *fileHeader, String saveFileName, FileIdentifier identifier);
bool checkFileHeaderVersion(FileHeader *fileHeader, String saveFileName, u8 currentVersion);

template <typename T>
bool readArray(FileArray source, u8 *baseMemory, Array<T> *dest);

String readString(FileString source, u8 *base);
void rleDecode(u8 *source, u8 *dest, s32 destSize);

bool decodeBlob(FileBlob blob, u8 *baseMemory, u8 *dest, s32 destSize);
bool decodeBlob(FileBlob blob, u8 *baseMemory, Array2<u8> *dest);

struct FileWriter
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
};

FileWriter startWritingFile(FileIdentifier identifier, u8 version);
