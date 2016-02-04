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
	bool isPath;
};

// Farming stuff
enum BuildingArchetype {
	BA_None = -1,

	BA_Field = 0,
	BA_Barn = 1,
	BA_Farmhouse,
	BA_Path,

	BA_Count
};
BuildingDefinition buildingDefinitions[] = {
	// size, name, 		 image, 				 costs b/d,  isPath
	{4,4, 	"Field", 	 TextureAtlasItem_Field, 200, 20,	 false},
	{4,4, 	"Barn", 	 TextureAtlasItem_Barn,  2000, 1000, false},
	{4,4, 	"Farmhouse", TextureAtlasItem_House, 2000, 1000, false},
	{1,1, 	"Path", 	 TextureAtlasItem_Path,  10, 10,	 true},
};

enum FieldState {
	FieldState_Empty = 0,
	FieldState_Planting,
	FieldState_Growing,
	FieldState_Grown,
	FieldState_Harvesting,
	FieldState_Gathering,
};
struct FieldData {
	FieldState state;
	int32 progress;
	int32 progressCounter;
};

struct Building {
	BuildingArchetype archetype;
	Rect footprint;
	union {
		FieldData field;
	};
	Building *prevOfType;
	Building *nextOfType;
};
const int32 fieldPlantCost = 500;
const int fieldWidth = 4;
const int fieldHeight = 4;
const int fieldSize = fieldWidth * fieldHeight;
const int fieldMaxGrowth = fieldSize*3;
const int fieldProgressToPlant = 1;
const int fieldProgressToHarvest = 1;

const V2 potatoCarryOffset = v2(-4.0f/16.0f, -14.0f/16.0f);

struct Worker
{
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

struct PathLayer {
	int32 pathGroupCount;
	int32 *data; // Represents the pathing 'group'. 0 = unpathable, >0 = any tile with the same value is connected
};

struct City {
	char *name;
	int32 funds;
	int32 monthlyExpenditure;

	int32 width, height;
	Terrain *terrain;
	PathLayer pathLayer;

	uint32 buildingCount;
	uint32 buildingCountMax;
	Building buildings[1024]; // TODO: Make the number of buildings unlimited!
	uint32 *tileBuildings; // Map from x,y -> building id at that location.
	// Building IDs are 1-indexed (0 meaning null).

	Building *firstBuildingOfType[BA_Count];

	// Workers!
	uint32 workerCount;
	Worker workers[512]; // TODO: Decide on number of workers!
	JobBoard jobBoard;
};

inline uint32 tileIndex(City *city, int32 x, int32 y) {
	return (y * city->width) + x;
}

inline bool tileExists(City *city, int32 x, int32 y) {
	return (x >= 0) && (x < city->width)
		&& (y >= 0) && (y < city->height);
}

inline Terrain terrainAt(City *city, int32 x, int32 y) {
	if (!tileExists(city, x, y)) return Terrain_Invalid;
	return city->terrain[tileIndex(city, x, y)];
}

inline Building* getBuildingByID(City *city, uint32 buildingID) {
	if (buildingID <= 0 || buildingID > city->buildingCountMax) {
		return null;
	}

	return &(city->buildings[buildingID]);
}

inline Building* getBuildingAtPosition(City *city, Coord position) {
	return getBuildingByID(city, city->tileBuildings[tileIndex(city,position.x,position.y)]);
}