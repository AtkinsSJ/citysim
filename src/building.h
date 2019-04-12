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
	u32 typeID;

	union
	{
		V2I size;
		struct
		{
			s32 width;
			s32 height;
		};
	};
	u32 textureAssetType;
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
	u32 canBeBuiltOnID;
	u32 buildOverResult;

	enum ZoneType growsInZone;

	s32 demolishCost;

	s32 residents;
	s32 jobs;
	
	bool isPath;
	bool carriesPower;
	s32 power; // Positive for production, negative for consumption
};

ChunkedArray<BuildingDef> buildingDefs = {};

struct Building
{
	u32 id;
	u32 typeID;
	Rect2I footprint;
	u32 textureRegionOffset; // used as the offset for getTextureRegionID
	
	s32 currentResidents;
	s32 currentJobs;
};

void loadBuildingDefs(ChunkedArray<BuildingDef> *buildings, AssetManager *assets, Asset *asset);

// Returns 0 if not found
s32 findBuildingTypeByName(String name)
{
	s32 result = 0;

	// TODO: Use an iterator instead, it's faster!
	for (s32 buildingType = 1; buildingType < buildingDefs.count; buildingType++)
	{
		BuildingDef *def = get(&buildingDefs, buildingType);
		if (equals(def->name, name))
		{
			result = buildingType;
			break;
		}
	}

	return result;
}