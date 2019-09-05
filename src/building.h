#pragma once

enum BuildMethod
{
	BuildMethod_None,
	BuildMethod_Paint,
	BuildMethod_Plop,
	BuildMethod_DragRect,
	BuildMethod_DragLine,
};

enum BuildingFlag
{
	Building_CarriesPower,
	Building_RequiresTransportConnection,

	BuildingFlagCount
};

struct BuildingDef
{
	String name;
	s32 typeID;

	Flags8<BuildingFlag> flags;

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
	
	Flags_TransportType transportTypes;

	s32 power; // Positive for production, negative for consumption

	EffectRadius landValueEffect;

	EffectRadius pollutionEffect;
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

enum BuildingProblem
{
	BuildingProblem_NoPower,
	BuildingProblem_NoTransportAccess,

	BuildingProblemCount
};

String buildingProblemNames[BuildingProblemCount] = {
	makeString("No power!"),
	makeString("No access to transport!")
};

struct Building
{
	u32 id;
	s32 typeID;
	Rect2I footprint;
	s32 spriteOffset; // used as the offset for getSprite
	
	s32 currentResidents;
	s32 currentJobs;

	Flags8<BuildingProblem> problems;
};

// NB: If someone needs a pointer to a building across multiple frames, use one of these references.
// The position lets you look-up the building via getBuildingAt(), and the buildingID lets you check
// that the found building is indeed the one you were after.
// You'd then do something like this:
//     Building *theBuilding = getBuilding(city, buildingRef);
// which would look-up the building, check its ID, and return the Building* if it matches
// and null if it doesn't, or if no building is at the position any more.
// - Sam, 19/08/2019
struct BuildingRef
{
	u32 buildingID;
	V2I buildingPos;
};
BuildingRef getReferenceTo(Building *building);
Building *getBuilding(City *city, BuildingRef ref);

BuildingDef *getBuildingDef(Building *building);

void loadBuildingDefs(Blob data, Asset *asset);
void refreshBuildingSpriteCache(BuildingCatalogue *catalogue);
BuildingDef *getBuildingDef(s32 buildingTypeID);
BuildingDef *findBuildingDef(String name);

s32 getRequiredPower(Building *building);


// TODO: These are a bit hacky... I want to hide the implementation details of the catalogue, but
// creating a whole set of iterator stuff which is almost identical to the regular iterators seems
// a bit excessive?
// I guess maybe I could use one that would work for all of these. eh, maybe worth trying later.
// - Sam, 15/06/2019
ChunkedArray<BuildingDef *> *getConstructibleBuildings();

template<typename Filter>
BuildingDef *findRandomZoneBuilding(ZoneType zoneType, Random *random, Filter filter);

s32 getMaxBuildingSize(ZoneType zoneType);

struct City;
void updateBuildingTexture(City *city, Building *building, BuildingDef *def = null);
void updateAdjacentBuildingTextures(City *city, Rect2I footprint);
