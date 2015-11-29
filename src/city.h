#pragma once

enum Terrain {
	Terrain_Invalid = 0,
	Terrain_Ground,
	Terrain_Water,
	Terrain_Forest,
	Terrain_Size
};
const int forestDemolishCost = 100;

struct BuildingDefinition {
	union {
		Coord size;
		struct {int32 width, height;};
	};
	string name;
	TextureAtlasItem textureAtlasItem;
	int32 buildCost;
	int32 demolishCost;
};

// Farming stuff
enum BuildingArchetype {
	BA_None = -1,

	BA_Field = 0,
	BA_Barn = 1,
	BA_Farmhouse,

	BA_Count
};
BuildingDefinition buildingDefinitions[] = {
	{4,4, "Field", TextureAtlasItem_Field, 200, 20},
	{4,4, "Barn", TextureAtlasItem_Barn, 2000, 1000},
	{4,4, "Farmhouse", TextureAtlasItem_House, 2000, 1000},
};

// Goblin Fortress stuff
/* 
enum BuildingArchetype {
	BA_None = -1,
	BA_Hovel = 0,
	BA_Pit,
	BA_Paddock,
	BA_Butcher,
	BA_Road,
	BA_Count
};
BuildingDefinition buildingDefinitions[] = {
	{1,1, "Hovel", TextureAtlasItem_Hovel},
	{5,5, "Pit", TextureAtlasItem_Pit},
	{3,3, "Paddock", TextureAtlasItem_Paddock},
	{2,2, "Butcher", TextureAtlasItem_Butcher},
	{1,1, "Road", TextureAtlasItem_Road},
};
*/

enum FieldState {
	FieldState_Empty = 0,
	FieldState_Planting,
	FieldState_Growing,
	FieldState_Grown,
	FieldState_Harvesting,
};
struct FieldData {
	FieldState state;
	int32 progress;
	int32 progressCounter;
};

struct Building {
	bool exists;
	BuildingArchetype archetype;
	Rect footprint;
	union {
		FieldData field;
	};
};
const int32 fieldPlantCost = 500;
const int fieldWidth = 4;
const int fieldHeight = 4;
const int fieldSize = fieldWidth * fieldHeight;
const int fieldMaxGrowth = fieldSize*3;
const int fieldProgressToPlant = 1;
const int fieldProgressToHarvest = 1;

struct Potato {
	bool exists;
	RealRect bounds; // In tiles!
	// NB: bounds rather than position because of workerMoveTo()
};
const V2 potatoCarryOffset = v2(-4.0f/16.0f, -14.0f/16.0f);

struct Worker {
	bool exists;
	V2 pos; // Position at start of day
	Animator animator;

	bool isMoving;
	bool isAtDestination;
	V2 renderPos;
	V2 dayEndPos;
	real32 movementInterpolation;

	bool isCarryingPotato;

	Job job;
};
const int workerHireCost = 100;
const int workerMonthlyCost = workerHireCost / 2;

struct City {
	char *name;
	int32 funds;
	int32 monthlyExpenditure;

	int32 width, height;
	Terrain *terrain;

	uint32 buildingCount;
	uint32 buildingCountMax;
	Building buildings[256]; // TODO: Make the number of buildings unlimited!
	uint32 *tileBuildings; // Map from x,y -> building id at that location.
	// Building IDs are 1-indexed (0 meaning null), however they're still stored
	// from position 0!

	Building *farmhouse;

	uint32 barnCount;
	Building *barns[64];

	// Workers!
	uint32 workerCount;
	Worker workers[512]; // TODO: Decide on number of workers!
	JobBoard jobBoard;

	Potato potatoes[2048]; // TODO: We really need to organise proper storage for random junk
};

#include "city.cpp"