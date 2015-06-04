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
};

// Farming stuff
enum BuildingArchetype {
	BA_None = -1,
	BA_Field = 0,
	BA_Count
};
BuildingDefinition buildingDefinitions[] = {
	{4,4, "Field", TextureAtlasItem_Field},
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

struct FieldData {
	bool exists;
	bool hasPlants;
	int32 growth;
	int32 growthCounter;
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

	FieldData fieldData[256]; // TODO: Decide on field limit!
};

#include "city.cpp"