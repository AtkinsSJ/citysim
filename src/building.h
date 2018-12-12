#pragma once

struct BuildingDef
{
	union
	{
		V2I size;
		struct
		{
			s32 width;
			s32 height;
		};
	};
	String name;
	u32 textureAssetType;
	s32 buildCost;
	s32 demolishCost;
	bool isPath;
	bool isPloppable;
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

Array<BuildingDef> buildingDefs = {};

struct Building
{
	u32 typeID;
	Rect2I footprint;
	u32 textureRegionOffset; // used as the offset for getTextureRegionID
	// union {
	// 	FieldData field;
	// };
};

void loadBuildingDefs(Array<BuildingDef> *buildings, AssetManager *assets, File file);
