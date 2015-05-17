#pragma once

enum BuildingArchetype {
	BA_Hovel,
	BA_Pit,
	BA_Paddock,
	BA_Butcher,
	BA_Road,
	BA_COUNT
};

struct BuildingDefinition {
	int32 width, height;
	string name;
};

BuildingDefinition buildingDefinitions[] = {
	{1,1, "Hovel"},
	{5,5, "Pit"},
	{3,3, "Paddock"},
	{2,2, "Butcher"},
	{1,1, "Road"},
};

struct Building {
	BuildingArchetype archetype;
	Rect footprint;
};

#include "building.cpp"