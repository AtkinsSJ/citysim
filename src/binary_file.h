#pragma once

#pragma pack(push, 1)

typedef u32 SAVIdentifier;
inline SAVIdentifier operator"" _id(const char *chars, size_t length)
{
	ASSERT(length == 4);
	return *((u32*)chars);
}

struct SAVString
{
	leU32 length;
	leU32 relativeOffset;
};

enum SAVBlobCompressionScheme
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

struct SAVBlob
{
	leU32 length;
	leU32 decompressedLength;
	leU32 relativeOffset;
	leU32 compressionScheme;
};

struct SAVFileHeader
{
	LittleEndian<SAVIdentifier> identifier;
	leU8 version;

	// Bytes for checking for unwanted newline-conversion
	leU8 unixNewline = 0x0A;
	leU8 dosNewline[2] = {0x0D, 0x0A};

	SAVFileHeader(SAVIdentifier theIdentifier, leU8 theVersion)
		: identifier(theIdentifier), version(theVersion)
	{}
};

struct SAVChunkHeader
{
	LittleEndian<SAVIdentifier> identifier;
	leU8 version;
	leU8 _pad[3]; // For future use maybe? Mostly just to make this be a multiple of 4 bytes.
	leU32 length; // Length of the chunk, NOT including the size of this SAVChunkHeader
};

#pragma pack(pop)

bool fileHeaderIsValid(SAVFileHeader *fileHeader, String saveFileName, SAVIdentifier identifier);
bool checkFileHeaderVersion(SAVFileHeader *fileHeader, String saveFileName, u8 currentVersion);
String readString(SAVString source, u8 *base);
void rleDecode(u8 *source, u8 *dest, s32 destSize);

SAVBlob appendBlob(s32 currentOffset, WriteBuffer *buffer, s32 length, u8 *data, SAVBlobCompressionScheme scheme);
SAVBlob appendBlob(s32 currentOffset, WriteBuffer *buffer, Array2<u8> *data, SAVBlobCompressionScheme scheme);
bool decodeBlob(SAVBlob blob, u8 *baseMemory, u8 *dest, s32 destSize);
bool decodeBlob(SAVBlob blob, u8 *baseMemory, Array2<u8> *dest);
