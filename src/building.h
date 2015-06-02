#pragma once


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
};

#include "building.cpp"