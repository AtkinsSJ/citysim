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
	TextureAssetType textureAssetType;
	s32 buildCost;
	s32 demolishCost;
	bool isPath;
	bool isPloppable;
	bool carriesPower;
	s32 power; // Positive for production, negative for consumption
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

