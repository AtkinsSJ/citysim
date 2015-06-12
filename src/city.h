#pragma once

enum Terrain {
	Terrain_Invalid = 0,
	Terrain_Ground,
	Terrain_Water,
	Terrain_Size
};

struct BuildingDefinition {
	int32 width, height;
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

struct Building {
	bool exists;
	BuildingArchetype archetype;
	Rect footprint;
	void *data;
};

enum FieldState {
	FieldState_Empty = 0,
	FieldState_Planting,
	FieldState_Growing,
	FieldState_Grown,
	FieldState_Harvesting,
};
struct FieldData {
	bool exists;
	FieldState state;
	int32 growth;
	int32 growthCounter;
};
const int fieldSize = 16;
const int fieldMaxGrowth = fieldSize*3;

struct Worker {
	bool exists;
	V2 pos;
};
const int workerHireCost = 100;

enum JobType {
	JobType_None = -1,

	JobType_Plant = 0,
	JobType_Harvest = 1,

	JobTypeCount
};
struct Job {
	JobType type;
	Building *building;
};
struct JobBoard {
	Job jobs[128];
	int32 jobCount;
};

struct City {
	char *name;
	int32 funds;

	uint32 width, height;
	Terrain *terrain;

	uint32 buildingCount;
	uint32 buildingCountMax;
	Building buildings[256]; // TODO: Make the number of buildings unlimited!
	uint32 *tileBuildings; // Map from x,y -> building id at that location.
	// Building IDs are 1-indexed (0 meaning null), however they're still stored
	// from position 0!

	Building *farmhouse;

	FieldData fieldData[256]; // TODO: Decide on field limit!

	// Workers!
	Worker workers[128]; // TODO: Decide on number of workers!
	JobBoard jobBoard;
};

#include "city.cpp"