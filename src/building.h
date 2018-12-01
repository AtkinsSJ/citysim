#pragma once

// TODO: This currently has to match the order of the buildingDefinitions array (and in turn, the buildings.def file)
// So, we want to get rid of this!
enum BuildingArchetype
{
	BA_Road,
	BA_House_2x2,
	BA_Factory_3x3,

	BA_Count,
	BA_None = -1
};

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
	BuildingArchetype archetype;
	Rect2I footprint;
	u32 textureRegionOffset; // used as the offset for getTextureRegionID
	// union {
	// 	FieldData field;
	// };
};

