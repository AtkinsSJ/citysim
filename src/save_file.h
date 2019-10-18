#pragma once

// TODO: Endian-ness???!?!?!?!??!?!?!?!?!
// I have no idea how to figure out what's going on with that.
// OK, apparently x86 is little-endian, so it makes sense to enforce that as the
// format's choice, because it means not having to do any switching.
//
// Trouble is, I'm not sure how I'd implement switching. It can't happen inside
// WriteBuffer because that doesn't know anything about the data it receives.
// That means reordering the bytes of each individual member of each struct...
//
// GCC has:
//       #pragma scalar_storage_order little-endian
// which would be good! But I don't think VC++ has an equivalent, and a quick look
// online, it seems G++ doesn't handle it either. :'(
//
// There's a header file that'll do the conversions automatically based on using
// their custom leFOO / beFOO types: https://github.com/tatewake/endian-template/blob/master/tEndian.h
// That might be good, though I haven't had time to read up on it yet.
//
// OK, I've looked at that header file and it's pretty simple really, so I think I'll
// implement it myself!
// You just create a template type that detects the CPU's endianness, and then swaps the
// byte order whenever you assign to or from that type.


#pragma pack(push, 1)

struct SAVString
{
	u32 length;
	u32 relativeOffset;
};

const u8 SAV_VERSION = 1;

struct SAVFileHeader
{
	u8 identifier[4];
	u8 version;

	// Bytes for checking for unwanted newline-conversion
	u8 unixNewline;
	u8 dosNewline[2];

	SAVFileHeader() :
		identifier{'C','I','T','Y'},
		version(SAV_VERSION),
		unixNewline(0x0A),
		dosNewline{0x0D, 0x0A}
	{ }
};

struct SAVChunkHeader
{
	u8 identifier[4];
	u8 version;
	u8 _pad[3]; // For future use maybe? Mostly just to make this be a multiple of 4 bytes.
	u32 length; // Length of the chunk, NOT including the size of this SAVChunkHeader
};

const u8 SAV_META_VERSION = 1;
const u8 SAV_META_ID[4] = {'M', 'E', 'T', 'A'};
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
	u32 buildingTypeCount;
	u32 offsetForBuildingTypeTable; // Map from Building string ID to to the int id used in the type array below.
	// The Buildings table is just a sequence of (u32 id, u32 length, then `length` bytes for the characters)

	u32 highestBuildingID;

	// Array of the buildings in the city, as SAVBuildings
	u32 buildingCount;
	u32 offsetForBuildingArray;
};
struct SAVBuilding
{
	u32 id;
	u32 typeID;

	u16 x;
	u16 y;
	u16 w;
	u16 h;

	u16 spriteOffset;
	u16 currentResidents;
	u16 currentJobs;

	// TODO: Record builidng problems somehow!
};

const u8 SAV_CRIM_VERSION = 1;
const u8 SAV_CRIM_ID[4] = {'C', 'R', 'I', 'M'};
struct SAVChunk_Crime
{
	u32 totalJailCapacity;
	u32 occupiedJailCapacity;
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
	u32 activeFireCount;
	u32 offsetForActiveFires; // Array of SAVFires

	// TODO: Fire service building data
};
struct SAVFire
{
	u16 x;
	u16 y;
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
	u32 offsetForTileLandValue; // Array of u8s.
};

const u8 SAV_PLTN_VERSION = 1;
const u8 SAV_PLTN_ID[4] = {'P', 'L', 'T', 'N'};
struct SAVChunk_Pollution
{
	// TODO: Maybe RLE this, but I'm not sure. It's probably pretty variable.
	u32 offsetForTilePollution; // Array of u8s
};

const u8 SAV_TERR_VERSION = 1;
const u8 SAV_TERR_ID[4] = {'T', 'E', 'R', 'R'};
struct SAVChunk_Terrain
{
	u32 terrainTypeCount;
	u32 offsetForTerrainTypeTable; // Map from terrain string ID to to the int id used in the type array below.
	// The terrain table is just a sequence of (u32 id, u32 length, then `length` bytes for the characters)

	u32 offsetForTileTerrainType;  // Array of u8s    TODO: RLE?
	u32 offsetForTileHeight;       // Array of u8s
	u32 offsetForTileSpriteOffset; // Array of u8s
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
	u32 offsetForTileZone; // Array of u8s    TODO: RLE?
};

#pragma pack(pop)

bool writeSaveFile(FileHandle *file, City *city);
bool loadSaveFile(FileHandle *file, City *city, MemoryArena *gameArena);

//
// INTERNAL
//
bool identifiersAreEqual(const u8 *a, const u8 *b);
String loadString(SAVString source, u8 *base, MemoryArena *arena);
