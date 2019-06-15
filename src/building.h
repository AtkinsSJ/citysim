#pragma once

enum BuildMethod
{
	BuildMethod_None,
	BuildMethod_Paint,
	BuildMethod_Plop,
	BuildMethod_DragRect,
	BuildMethod_DragLine,
};

struct BuildingDef
{
	String name;
	s32 typeID;

	union
	{
		V2I size;
		struct
		{
			s32 width;
			s32 height;
		};
	};
	String spriteName;
	SpriteGroup *sprites;
	enum DataLayer linkTexturesLayer;

	BuildMethod buildMethod;
	s32 buildCost;
	/*
	 * This is a bit of a mess, and I don't have a good name for it!
	 * If we try and place this building over a buildingTypeThisCanBeBuiltOver,
	 * then buildOverResultBuildingType is produced.
	 * TODO: If this gets used a lot, we may want to instead have an outside list
	 * of "a + b -> c" building combinations. Right now, a building can only be an
	 * ingredient for one building, which I KNOW is not enough!
	 */
	s32 canBeBuiltOnID;
	s32 buildOverResult;

	enum ZoneType growsInZone;

	s32 demolishCost;

	s32 residents;
	s32 jobs;
	
	bool isPath;
	bool carriesPower;
	s32 power; // Positive for production, negative for consumption
};

struct BuildingCatalogue
{
	bool isInitialised;
	ChunkedArray<BuildingDef> buildingDefs;

	ChunkedArray<BuildingDef *> constructibleBuildings;
	ChunkedArray<BuildingDef *> rGrowableBuildings;
	ChunkedArray<BuildingDef *> cGrowableBuildings;
	ChunkedArray<BuildingDef *> iGrowableBuildings;

	s32 maxRBuildingDim;
	s32 maxCBuildingDim;
	s32 maxIBuildingDim;
};

BuildingCatalogue buildingCatalogue = {};

struct Building
{
	u32 id;
	s32 typeID;
	Rect2I footprint;
	s32 spriteOffset; // used as the offset for getSprite
	
	s32 currentResidents;
	s32 currentJobs;
};


void loadBuildingDefs(AssetManager *assets, Blob data, Asset *asset);
void refreshBuildingSpriteCache(BuildingCatalogue *catalogue, AssetManager *assets);
BuildingDef *getBuildingDef(s32 buildingTypeID);

// TODO: These are a bit hacky... I want to hide the implementation details of the catalogue, but
// creating a whole set of iterator stuff which is almost identical to the regular iterators seems
// a bit excessive?
// I guess maybe I could use one that would work for all of these. eh, maybe worth trying later.
// - Sam, 15/06/2019
ChunkedArray<BuildingDef *> *getConstructibleBuildings();
ChunkedArray<BuildingDef *> *getRGrowableBuildings();
ChunkedArray<BuildingDef *> *getCGrowableBuildings();
ChunkedArray<BuildingDef *> *getIGrowableBuildings();


struct City;
void updateBuildingTexture(City *city, Building *building, BuildingDef *def = null);
void updateAdjacentBuildingTextures(City *city, Rect2I footprint);

// Returns 0 if not found
// TODO: use a hashmap!
s32 findBuildingTypeByName(String name)
{
	DEBUG_FUNCTION();
	
	s32 result = 0;

	// TODO: Use an iterator instead, it's faster!
	for (s32 buildingType = 1; buildingType < buildingCatalogue.buildingDefs.count; buildingType++)
	{
		BuildingDef *def = get(&buildingCatalogue.buildingDefs, buildingType);
		if (equals(def->name, name))
		{
			result = buildingType;
			break;
		}
	}

	return result;
}
