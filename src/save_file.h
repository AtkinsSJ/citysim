#pragma once

// NB: This is inspired by https://github.com/tatewake/endian-template/blob/master/tEndian.h
// However, we require a *very small* subset of that! Just assignment to/from LE types from
// their native equivalents. So, I implemented it myself.

inline bool cpuIsLittleEndian()
{
	u16 i = 1;

	return (*((u8*) &i) == 1);
}

template<typename T>
T reverseBytes(T input)
{
	T result;

	u8 *inputBytes  = (u8*) &input;
	u8 *outputBytes = (u8*) &result;

	switch(sizeof(T))
	{
		case 8:
		{
			outputBytes[0] = inputBytes[7];
			outputBytes[1] = inputBytes[6];
			outputBytes[2] = inputBytes[5];
			outputBytes[3] = inputBytes[4];
			outputBytes[4] = inputBytes[3];
			outputBytes[5] = inputBytes[2];
			outputBytes[6] = inputBytes[1];
			outputBytes[7] = inputBytes[0];
		} break;

		case 4:
		{
			outputBytes[0] = inputBytes[3];
			outputBytes[1] = inputBytes[2];
			outputBytes[2] = inputBytes[1];
			outputBytes[3] = inputBytes[0];
		} break;

		case 2:
		{
			outputBytes[0] = inputBytes[1];
			outputBytes[1] = inputBytes[0];
		} break;

		default:
		{
			ASSERT(false); // Only basic types are supported
		} break;
	}

	return result;
}

template<typename T>
struct LittleEndian
{
	T data;

	//
	// SET
	//
	// Direct assignment
	LittleEndian(T value) : data(cpuIsLittleEndian() ? value : reverseBytes(value))
	{}

	LittleEndian() : data() {}

	//
	// GET
	//
	operator T() const { return cpuIsLittleEndian() ? data : reverseBytes(data); }
};

typedef u8                leU8; // Just for consistency
typedef LittleEndian<u16> leU16;
typedef LittleEndian<u32> leU32;
typedef LittleEndian<u64> leU64;

typedef s8                leS8; // Just for consistency
typedef LittleEndian<s16> leS16;
typedef LittleEndian<s32> leS32;
typedef LittleEndian<s64> leS64;

typedef LittleEndian<f32> leF32;
typedef LittleEndian<f64> leF64;

//
// A crazy, completely-unnecessary idea: we could implement a thumbnail handler so that
// saved games show a little picture of the city. Probably we'd embed a small rendered
// image of the city in the save file (which would be useful for the in-game load menu)
// and then just use that.
// See https://docs.microsoft.com/en-us/windows/win32/shell/thumbnail-providers
// This is very clearly not a good use of time but it would be SUPER COOL.
// - Sam, 23/10/2019
//

#pragma pack(push, 1)

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

const u8 SAV_VERSION = 1;

struct SAVFileHeader
{
	leU8 identifier[4];
	leU8 version;

	// Bytes for checking for unwanted newline-conversion
	leU8 unixNewline;
	leU8 dosNewline[2];

	SAVFileHeader() :
		identifier{'C','I','T','Y'},
		version(SAV_VERSION),
		unixNewline(0x0A),
		dosNewline{0x0D, 0x0A}
	{ }
};

struct SAVChunkHeader
{
	leU8 identifier[4];
	leU8 version;
	leU8 _pad[3]; // For future use maybe? Mostly just to make this be a multiple of 4 bytes.
	leU32 length; // Length of the chunk, NOT including the size of this SAVChunkHeader
};

//
//
//
//         Start of actual game-related stuff
//
//
//

const u8 SAV_META_VERSION = 1;
const u8 SAV_META_ID[4] = {'M', 'E', 'T', 'A'};
struct SAVChunk_Meta
{
	leU64 saveTimestamp; // Unix timestamp
	leU16 cityWidth;
	leU16 cityHeight;
	leS32 funds;
	SAVString cityName;
	SAVString playerName;
	leU32 population;
	leU32 jobs;

	// Clock
	LittleEndian<GameTimestamp> currentDay;
	leF32 timeWithinDay;

	// Camera
	leF32 cameraX;
	leF32 cameraY;
	leF32 cameraZoom;
};

struct SAVChunk_Mods
{
	// List which mods are enabled in this save, so that we know if they're present or not!
	// Also, probably turn individual mods on/off to match the save game, that'd be useful!

	// Hmmm... I guess we may also want to allow mods to add their own chunks. That's a bit
	// trickier! Well, that's only if mods can have their own code somehow - I know we'll
	// want to support custom content that's just data, but scripting is a whole other thing.
	// Just-data mods won't need extra sections.

	// Another thought: we could just pack mod-chunk data inside the MODS data maybe. Just
	// have sub-chunks within it. Again, this is very far away, so just throwing ideas out,
	// but if we store enabled mods as:
	// (name, data size, data offset)
	// then they can have any amount of data they like, they just have to save/load to a
	// binary blob.
};

const u8 SAV_BDGT_VERSION = 1;
const u8 SAV_BDGT_ID[4] = {'B', 'D', 'G', 'T'};
struct SAVChunk_Budget
{
	// TODO: Budget things
};

const u8 SAV_BLDG_VERSION = 1;
const u8 SAV_BLDG_ID[4] = {'B', 'L', 'D', 'G'};
struct SAVChunk_Buildings
{
	leU32 buildingTypeCount;
	leU32 offsetForBuildingTypeTable; // Map from Building string ID to to the int id used in the type array below.
	// The Buildings table is just a sequence of (u32 id, u32 length, then `length` bytes for the characters)

	leU32 highestBuildingID;

	// Array of the buildings in the city, as SAVBuildings
	leU32 buildingCount;
	SAVBlob buildings;
};
struct SAVBuilding
{
	leU32 id;
	leU32 typeID;

	leU16 x;
	leU16 y;
	leU16 w;
	leU16 h;

	leU16 spriteOffset;
	leU16 currentResidents;
	leU16 currentJobs;
	leS16 variantIndex;

	// TODO: Record builidng problems somehow!
};

const u8 SAV_CRIM_VERSION = 1;
const u8 SAV_CRIM_ID[4] = {'C', 'R', 'I', 'M'};
struct SAVChunk_Crime
{
	leU32 totalJailCapacity;
	leU32 occupiedJailCapacity;
};

const u8 SAV_EDUC_VERSION = 1;
const u8 SAV_EDUC_ID[4] = {'E', 'D', 'U', 'C'};
struct SAVChunk_Education
{
	// Building education level, when that's implemented
};

const u8 SAV_FIRE_VERSION = 1;
const u8 SAV_FIRE_ID[4] = {'F', 'I', 'R', 'E'};
struct SAVChunk_Fire
{
	// Active fires
	leU32 activeFireCount;
	SAVBlob activeFires; // Array of SAVFires

	// TODO: Fire service building data
};
struct SAVFire
{
	leU16 x;
	leU16 y;
	// TODO: severity
};

const u8 SAV_HLTH_VERSION = 1;
const u8 SAV_HLTH_ID[4] = {'H', 'L', 'T', 'H'};
struct SAVChunk_Health
{
	// Building health level, when that's implemented
};

const u8 SAV_LVAL_VERSION = 1;
const u8 SAV_LVAL_ID[4] = {'L', 'V', 'A', 'L'};
struct SAVChunk_LandValue
{
	// Kind of redundant as it can be calculated fresh, but in case we have over-time
	// effects, we might as well put this here.
	SAVBlob tileLandValue; // Array of u8s
};

const u8 SAV_PLTN_VERSION = 1;
const u8 SAV_PLTN_ID[4] = {'P', 'L', 'T', 'N'};
struct SAVChunk_Pollution
{
	// TODO: Maybe RLE this, but I'm not sure. It's probably pretty variable.
	SAVBlob tilePollution; // Array of u8s
};

const u8 SAV_TERR_VERSION = 1;
const u8 SAV_TERR_ID[4] = {'T', 'E', 'R', 'R'};
struct SAVChunk_Terrain
{
	leS32 terrainGenerationSeed;
	// TODO: Other terrain-generation parameters

	leU32 terrainTypeCount;
	leU32 offsetForTerrainTypeTable; // Map from terrain string ID to to the int id used in the type array below.
	// The terrain table is just a sequence of (u32 id, u32 length, then `length` bytes for the characters)

	SAVBlob tileTerrainType;  // Array of u8s (NB: We could make this be a larger type and then compact it down to u8s in the file if there are few enough...)
	SAVBlob tileHeight;       // Array of u8s
	SAVBlob tileSpriteOffset; // Array of u8s    TODO: This is a lot of data, can we just store RNG parameters instead, and then generate them on load?
};

const u8 SAV_TPRT_VERSION = 1;
const u8 SAV_TPRT_ID[4] = {'T', 'P', 'R', 'T'};
struct SAVChunk_Transport
{
	// TODO: Information about traffic density, routes, etc.
	// (Not sure what we'll actually simulate yet!)
};

const u8 SAV_ZONE_VERSION = 1;
const u8 SAV_ZONE_ID[4] = {'Z', 'O', 'N', 'E'};
struct SAVChunk_Zone
{
	SAVBlob tileZone; // Array of u8s
};

#pragma pack(pop)

struct GameState;
bool writeSaveFile(FileHandle *file, GameState *gameState);
bool loadSaveFile(FileHandle *file, GameState *gameState);

//
// INTERNAL
//
bool fileHeaderIsValid(SAVFileHeader *fileHeader, String saveFileName);
bool checkFileHeaderVersion(SAVFileHeader *fileHeader, String saveFileName);
bool identifiersAreEqual(const u8 *a, const u8 *b);
String readString(SAVString source, u8 *base);
void rleDecode(u8 *source, u8 *dest, s32 destSize);

SAVBlob appendBlob(s32 currentOffset, WriteBuffer *buffer, s32 length, u8 *data, SAVBlobCompressionScheme scheme);
SAVBlob appendBlob(s32 currentOffset, WriteBuffer *buffer, Array2<u8> *data, SAVBlobCompressionScheme scheme);
bool decodeBlob(SAVBlob blob, u8 *baseMemory, u8 *dest, s32 destSize);
bool decodeBlob(SAVBlob blob, u8 *baseMemory, Array2<u8> *dest);
