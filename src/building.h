#pragma once

struct BuildingDefinition
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
};

Array<BuildingDefinition> buildingDefinitions = {};

struct Building
{
	u32 typeID;
	Rect2I footprint;
	u32 textureRegionOffset; // used as the offset for getTextureRegionID
	// union {
	// 	FieldData field;
	// };
};

