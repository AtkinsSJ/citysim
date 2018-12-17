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

	BuildMethod buildMethod;
	s32 buildCost;
	s32 demolishCost;
	
	bool isPath;
	bool carriesPower;
	s32 power; // Positive for production, negative for consumption

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
};

ChunkedArray<BuildingDef> buildingDefs = {};

struct Building
{
	u32 typeID;
	Rect2I footprint;
	u32 textureRegionOffset; // used as the offset for getTextureRegionID
	// union {
	// 	FieldData field;
	// };
};

void loadBuildingDefs(ChunkedArray<BuildingDef> *buildings, AssetManager *assets, File file);

// Returns 0 if not found
u32 findBuildingTypeByName(String name)
{
	u32 result = 0;

	for (u32 buildingType = 1; buildingType < buildingDefs.itemCount; buildingType++)
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