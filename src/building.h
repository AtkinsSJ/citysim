#pragma once

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

// BuildingDefinition buildingDefinitions[] = {
// 	// size, name, 		 image, 				 costs b/d,  isPath
// 	{1,1, "Road", TextureAssetType_Road, 10, 10, true},
// 	{2,2, "House", TextureAssetType_House_2x2, 20, 50, false},
// 	{3,3, "Factory", TextureAssetType_Factory_3x3, 40, 100, false},
// };

struct Building
{
	BuildingArchetype archetype;
	Rect2I footprint;
	u32 textureRegionOffset; // used as the offset for getTextureRegionID
	// union {
	// 	FieldData field;
	// };
};

