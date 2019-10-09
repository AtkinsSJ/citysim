#pragma once

#pragma pack(push, 1)

struct SAVFileHeader
{
	u8 identifier[4];
	u8 version;

	// TODO: Bytes for checking for unwanted newline-conversion
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
	// TODO: Somehow put the city name and player name here!
	// Not sure if I want to do fixed-length strings, or put the lengths here and then make them semi-limitless?
};

struct SAVChunk_Budget
{

};

struct SAVChunk_Buildings
{

};

struct SAVChunk_Crime
{

};

struct SAVChunk_Education
{

};

struct SAVChunk_Fire
{

};

struct SAVChunk_Health
{

};

struct SAVChunk_LandValue
{

};

struct SAVChunk_Pollution
{

};

struct SAVChunk_Terrain
{

};

struct SAVChunk_Transport
{

};

struct SAVChunk_Zone
{

};

#pragma pack(pop)


