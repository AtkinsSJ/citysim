/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "binary_file.h"
#include "game_clock.h"
#include <Util/Basic.h>

struct FileHandle;

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

u8 const SAV_VERSION = 1;
FileIdentifier const SAV_FILE_ID = "CITY"_id;

u8 const SAV_META_VERSION = 1;
FileIdentifier const SAV_META_ID = "META"_id;
struct SAVSection_Meta {
    leU64 saveTimestamp; // Unix timestamp
    leU16 cityWidth;
    leU16 cityHeight;
    leS32 funds;
    FileString cityName;
    FileString playerName;
    leU32 population;
    leU32 jobs;

    // Clock
    LittleEndian<GameTimestamp> currentDate;
    leF32 timeWithinDay;

    // Camera
    leF32 cameraX;
    leF32 cameraY;
    leF32 cameraZoom;
};

struct SAVSection_Mods {
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

u8 const SAV_BUDGET_VERSION = 1;
FileIdentifier const SAV_BUDGET_ID = "BDGT"_id;
struct SAVSection_Budget {
    // TODO: Budget things
};

u8 const SAV_BUILDING_VERSION = 1;
FileIdentifier const SAV_BUILDING_ID = "BLDG"_id;
struct SAVSection_Buildings {
    FileArray buildingTypeTable; // SAVBuildingTypeEntry

    leU32 highestBuildingID;

    // Array of the buildings in the city, as SAVBuildings
    leU32 buildingCount;
    FileBlob buildings;
};
struct SAVBuildingTypeEntry {
    leU32 typeID;
    FileString name;
};
struct SAVBuilding {
    leU32 id;
    leU32 typeID;

    LittleEndian<GameTimestamp> creationDate;

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

u8 const SAV_CRIME_VERSION = 1;
FileIdentifier const SAV_CRIME_ID = "CRIM"_id;
struct SAVSection_Crime {
    leU32 totalJailCapacity;
    leU32 occupiedJailCapacity;
};

u8 const SAV_EDUCATION_VERSION = 1;
FileIdentifier const SAV_EDUCATION_ID = "EDUC"_id;
struct SAVSection_Education {
    // Building education level, when that's implemented
};

u8 const SAV_FIRE_VERSION = 1;
FileIdentifier const SAV_FIRE_ID = "FIRE"_id;
struct SAVSection_Fire {
    // Active fires
    leU32 activeFireCount;
    FileArray activeFires; // Array of SAVFires

    // TODO: Fire service building data
};
struct SAVFire {
    leU16 x;
    leU16 y;
    LittleEndian<GameTimestamp> startDate;
    // TODO: severity
};

u8 const SAV_HEALTH_VERSION = 1;
FileIdentifier const SAV_HEALTH_ID = "HLTH"_id;
struct SAVSection_Health {
    // TODO: Building health level, when that's implemented
};

u8 const SAV_LANDVALUE_VERSION = 1;
FileIdentifier const SAV_LANDVALUE_ID = "LVAL"_id;
struct SAVSection_LandValue {
    // Kind of redundant as it can be calculated fresh, but in case we have over-time
    // effects, we might as well put this here.
    FileBlob tileLandValue; // Array of u8s
};

u8 const SAV_POLLUTION_VERSION = 1;
FileIdentifier const SAV_POLLUTION_ID = "PLTN"_id;
struct SAVSection_Pollution {
    // TODO: Maybe RLE this, but I'm not sure. It's probably pretty variable.
    FileBlob tilePollution; // Array of u8s
};

u8 const SAV_TERRAIN_VERSION = 1;
FileIdentifier const SAV_TERRAIN_ID = "TERR"_id;
struct SAVSection_Terrain {
    leS32 terrainGenerationSeed;
    // TODO: Other terrain-generation parameters

    FileArray terrainTypeTable; // SAVTerrainTypeEntry

    FileBlob tileTerrainType;  // Array of u8s (NB: We could make this be a larger type and then compact it down to u8s in the file if there are few enough...)
    FileBlob tileHeight;       // Array of u8s
    FileBlob tileSpriteOffset; // Array of u8s    TODO: This is a lot of data, can we just store RNG parameters instead, and then generate them on load?
};
struct SAVTerrainTypeEntry {
    leU32 typeID;
    FileString name;
};

u8 const SAV_TRANSPORT_VERSION = 1;
FileIdentifier const SAV_TRANSPORT_ID = "TPRT"_id;
struct SAVSection_Transport {
    // TODO: Information about traffic density, routes, etc.
    // (Not sure what we'll actually simulate yet!)
};

u8 const SAV_ZONE_VERSION = 1;
FileIdentifier const SAV_ZONE_ID = "ZONE"_id;
struct SAVSection_Zone {
    FileBlob tileZone; // Array of u8s
};

#pragma pack(pop)

struct GameState;
bool writeSaveFile(FileHandle* file, GameState* gameState);
bool loadSaveFile(FileHandle* file, GameState* gameState);
