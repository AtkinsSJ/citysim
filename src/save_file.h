#pragma once

const u8 SAV_VERSION = 1; 

#pragma pack(push, 1)

struct SAVString
{
	u32 length;
	u32 relativeOffset;
};

struct SAVFileHeader
{
	u8 identifier[4];
	u8 version;

	// Bytes for checking for unwanted newline-conversion
	u8 unixNewline;
	u8 dosNewline[2];

	SAVFileHeader() :
		version(SAV_VERSION),
		identifier{'C','I','T','Y'},
		unixNewline(0x0A),
		dosNewline{0x0D, 0x0A}
	{ }
};

struct SAVChunkHeader
{
	u8 identifier[4];
	u8 version;
	u32 length; // Length of the chunk, NOT including the size of this SAVChunkHeader
};

struct SAVChunk_Meta
{
	u64 saveTimestamp; // Unix timestamp
	u16 cityWidth;
	u16 cityHeight;
	s32 funds;
	SAVString cityName;
	SAVString playerName;
};

struct SAVChunk_Mods
{
	// List which mods are enabled in this save, so that we know if they're present or not!
	// Also, probably turn individual mods on/off to match the save game, that'd be useful!

	// Hmmm... I guess we may also want to allow mods to add their own chunks. That's a bit
	// trickier! Well, that's only if mods can have their own code somehow - I know we'll
	// want to support custom content that's just data, but scripting is a whole other thing.
	// Just-data mods won't need extra sections.
};

struct SAVChunk_Budget
{

};

struct SAVChunk_Buildings
{
	// Some kind of map from BuilidngDef string id to the s32 typeID, so it survives changes
	// List of the buildings in the city
};

struct SAVChunk_Crime
{

};

struct SAVChunk_Education
{
	// Building education level, when that's implemented
};

struct SAVChunk_Fire
{
	// Active fires
};

struct SAVChunk_Health
{
	// Building health level, when that's implemented
};

struct SAVChunk_LandValue
{
	// Do we even need this?
};

struct SAVChunk_Pollution
{
	// Do we even need this? YES because of the over-time element
};

struct SAVChunk_Terrain
{
	// Map from terrain string ID to to the int id used in the type array below.
	// Tile type
	// Tile height
	// Tile sprite offset
};

struct SAVChunk_Transport
{

};

struct SAVChunk_Zone
{
	// Tile zone
};

#pragma pack(pop)

bool writeSaveFile(City *city, FileHandle *file);
