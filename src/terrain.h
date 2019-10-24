#pragma once

struct TerrainDef
{
	u8 typeID;
	String id;
	String nameTextID;

	String spriteName;
	SpriteGroup *sprites;

	bool canBuildOn;
};

struct TerrainCatalogue
{
	OccupancyArray<TerrainDef> terrainDefs;

	HashTable<TerrainDef *> terrainDefsByID;

	HashTable<u8> terrainIDToOldType;
	HashTable<u8> terrainIDToType;

	bool terrainDefsHaveChanged;
};

TerrainCatalogue terrainCatalogue = {};

struct TerrainLayer
{
	s32 terrainGenerationSeed;

	u8 *tileTerrainType;
	u8 *tileHeight;
	u8 *tileDistanceToWater;

	u8 *tileSpriteOffset;
};

void initTerrainLayer(TerrainLayer *layer, City *city, MemoryArena *gameArena);

void initTerrainCatalogue();
void loadTerrainDefs(Blob data, Asset *asset);
void refreshTerrainSpriteCache(TerrainCatalogue *catalogue);
void removeTerrainDefs(Array<String> idsToRemove);

void generateTerrain(City *city, Random *gameRandom);
void drawTerrain(City *city, Rect2I visibleArea, s8 shaderID);
TerrainDef *getTerrainAt(City *city, s32 x, s32 y);
u8 getDistanceToWaterAt(City *city, s32 x, s32 y);

void remapTerrainTypes(TerrainLayer *layer);

// Returns 0 if not found
u8 findTerrainTypeByID(String id);
