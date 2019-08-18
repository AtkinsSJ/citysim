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
	ChunkedArray<BuildingDef> allBuildings;
	HashTable<BuildingDef *> buildingsByName;

	ChunkedArray<BuildingDef *> constructibleBuildings;
	ChunkedArray<BuildingDef *> rGrowableBuildings;
	ChunkedArray<BuildingDef *> cGrowableBuildings;
	ChunkedArray<BuildingDef *> iGrowableBuildings;

	s32 maxRBuildingDim;
	s32 maxCBuildingDim;
	s32 maxIBuildingDim;
	s32 overallMaxBuildingDim;
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


void loadBuildingDefs(Blob data, Asset *asset);
void refreshBuildingSpriteCache(BuildingCatalogue *catalogue);
BuildingDef *getBuildingDef(s32 buildingTypeID);
BuildingDef *findBuildingDef(String name);

// TODO: These are a bit hacky... I want to hide the implementation details of the catalogue, but
// creating a whole set of iterator stuff which is almost identical to the regular iterators seems
// a bit excessive?
// I guess maybe I could use one that would work for all of these. eh, maybe worth trying later.
// - Sam, 15/06/2019
ChunkedArray<BuildingDef *> *getConstructibleBuildings();

// NB: Pass -1 for min/max residents/jobs to say you don't care
BuildingDef *findGrowableBuildingDef(Random *random, ZoneType zoneType, V2I maxSize, s32 minResidents, s32 maxResidents, s32 minJobs, s32 maxJobs);

struct City;
void updateBuildingTexture(City *city, Building *building, BuildingDef *def = null);
void updateAdjacentBuildingTextures(City *city, Rect2I footprint);
