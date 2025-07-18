/*
 * Copyright (c) 2018-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "entity.h"
#include "game_clock.h"
#include "tile_utils.h"
#include "transport.h"
#include <Sim/BuildingRef.h>
#include <Util/Flags.h>
#include <Util/OccupancyArray.h>
#include <Util/StringTable.h>

struct City;

enum class BuildMethod : u8 {
    None,
    Paint,
    Plop,
    DragRect,
    DragLine,
};

enum BuildingFlag {
    Building_CarriesPower,
    Building_RequiresTransportConnection,

    BuildingFlagCount
};

enum ConnectionDirection {
    Connect_N = 0,
    Connect_NE = 1,
    Connect_E = 2,
    Connect_SE = 3,
    Connect_S = 4,
    Connect_SW = 5,
    Connect_W = 6,
    Connect_NW = 7,

    ConnectionDirectionCount
};

enum ConnectionType {
    ConnectionType_Nothing,
    ConnectionType_Building1,
    ConnectionType_Building2,
    ConnectionType_Anything,

    ConnectionType_Invalid = -1
};

struct BuildingVariant {
    ConnectionType connections[ConnectionDirectionCount];

    String spriteName;
};

enum ZoneType : u8;

struct BuildingDef {
    String name;
    s32 typeID;

    String textAssetName;

    Flags8<BuildingFlag> flags;

    union {
        V2I size;
        struct
        {
            s32 width;
            s32 height;
        };
    };
    String spriteName;
    Array<BuildingVariant> variants;

    BuildMethod buildMethod;
    s32 buildCost;

    bool isIntersection;
    String intersectionPart1Name;
    String intersectionPart2Name;
    s32 intersectionPart1TypeID;
    s32 intersectionPart2TypeID;

    ZoneType growsInZone;

    s32 demolishCost;

    s32 residents;
    s32 jobs;

    Flags_TransportType transportTypes;

    s32 power; // Positive for production, negative for consumption

    EffectRadius landValueEffect;

    EffectRadius pollutionEffect;

    f32 fireRisk; // Defaults to 1.0
    EffectRadius fireProtection;

    EffectRadius healthEffect;

    EffectRadius policeEffect;
    s32 jailCapacity;

    // NB: When you add new fields here, make sure to add them to loadBuildingDefs(), and
    // copy their values when a building `extends` a template!
};

struct BuildingCatalogue {
    OccupancyArray<BuildingDef> allBuildings;
    HashTable<BuildingDef*> buildingsByName;
    StringTable buildingNames;

    HashTable<s32> buildingNameToTypeID;
    HashTable<s32> buildingNameToOldTypeID;

    ChunkedArray<BuildingDef*> constructibleBuildings;
    ChunkedArray<BuildingDef*> rGrowableBuildings;
    ChunkedArray<BuildingDef*> cGrowableBuildings;
    ChunkedArray<BuildingDef*> iGrowableBuildings;
    ChunkedArray<BuildingDef*> intersectionBuildings;

    s32 maxRBuildingDim;
    s32 maxCBuildingDim;
    s32 maxIBuildingDim;
    s32 overallMaxBuildingDim;
};

inline BuildingCatalogue buildingCatalogue = {};

enum BuildingProblemType {
    BuildingProblem_Fire,
    BuildingProblem_NoPower,
    BuildingProblem_NoTransportAccess,

    BuildingProblemCount
};

struct BuildingProblem {
    bool isActive;
    BuildingProblemType type;
    GameTimestamp startDate;
};

inline String buildingProblemNames[BuildingProblemCount] = {
    "building_problem_fire"_s,
    "building_problem_no_power"_s,
    "building_problem_no_transport"_s
};

s32 const NO_VARIANT = -1;
struct Building {
    u32 id;
    s32 typeID;
    GameTimestamp creationDate;
    Rect2I footprint;
    s32 variantIndex;

    Entity* entity;
    s32 spriteOffset; // used as the offset for getSprite

    s32 currentResidents;
    s32 currentJobs;

    s32 allocatedPower;

    BuildingProblem problems[BuildingProblemCount];
};

void initBuildingCatalogue();

BuildingDef* appendNewBuildingDef(String name);
void loadBuildingDefs(Blob data, Asset* asset);
void removeBuildingDefs(Array<String> idsToRemove);
BuildingDef* getBuildingDef(s32 buildingTypeID);
BuildingDef* getBuildingDef(Building* building);
BuildingDef* findBuildingDef(String name);

bool buildingDefHasType(BuildingDef* def, s32 typeID);

s32 getRequiredPower(Building* building);
bool buildingHasPower(Building* building);

void initBuilding(Building* building);
void initBuilding(Building* building, s32 id, BuildingDef* def, Rect2I footprint, GameTimestamp creationDate);
void updateBuilding(City* city, Building* building);
void addProblem(Building* building, BuildingProblemType problem);
void removeProblem(Building* building, BuildingProblemType problem);
bool hasProblem(Building* building, BuildingProblemType problem);

void loadBuildingSprite(Building* building);

ConnectionType connectionTypeOf(char c);
char asChar(ConnectionType connectionType);
bool matchesVariant(BuildingDef* def, BuildingVariant* variant, BuildingDef** neighbourDefs);

Maybe<BuildingDef*> findBuildingIntersection(BuildingDef* defA, BuildingDef* defB);

// TODO: These are a bit hacky... I want to hide the implementation details of the catalogue, but
// creating a whole set of iterator stuff which is almost identical to the regular iterators seems
// a bit excessive?
// I guess maybe I could use one that would work for all of these. eh, maybe worth trying later.
// - Sam, 15/06/2019
ChunkedArray<BuildingDef*>* getConstructibleBuildings();

s32 getMaxBuildingSize(ZoneType zoneType);

void updateBuildingVariant(City* city, Building* building, BuildingDef* def = nullptr);
void updateAdjacentBuildingVariants(City* city, Rect2I footprint);

void saveBuildingTypes();
void remapBuildingTypes(City* city);
