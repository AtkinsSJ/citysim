#pragma once

struct TerrainDef
{
	String name;

	String spriteName;
	SpriteGroup *sprites;

	bool canBuildOn;
};

ChunkedArray<TerrainDef> terrainDefs = {};

struct Terrain
{
	s32 type;
	s32 spriteOffset; // used as the offset for getSprite
};

Terrain invalidTerrain = {0, 0};

struct TerrainLayer
{
	Terrain *tileTerrain;
	u8 *tileTerrainHeight;
	u8 *tileDistanceToWater;
};

void initTerrainLayer(TerrainLayer *layer, City *city, MemoryArena *gameArena);

void loadTerrainDefs(ChunkedArray<TerrainDef> *terrains, Blob data, Asset *asset);
void refreshTerrainSpriteCache(ChunkedArray<TerrainDef> *terrains);

void generateTerrain(City *city);
void drawTerrain(City *city, Rect2I visibleArea, s8 shaderID);
TerrainDef *getTerrainAt(City *city, s32 x, s32 y);
u8 getDistanceToWaterAt(City *city, s32 x, s32 y);

// Returns 0 if not found
s32 findTerrainTypeByName(String name);
