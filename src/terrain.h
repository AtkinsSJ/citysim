#pragma once

struct TerrainDef
{
	String name;
	u8 typeID;
	
	String textAssetName;

	String spriteName;
	SpriteGroup *sprites;

	bool canBuildOn;
};

struct TerrainCatalogue
{
	OccupancyArray<TerrainDef> terrainDefs;

	HashTable<TerrainDef *> terrainDefsByName;

	HashTable<u8> terrainNameToOldType;
	HashTable<u8> terrainNameToType;
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
void removeTerrainDefs(Array<String> namesToRemove);
TerrainDef *getTerrainDef(u8 terrainType);

void generateTerrain(City *city, Random *gameRandom);
void drawTerrain(City *city, Rect2I visibleArea, s8 shaderID);
TerrainDef *getTerrainAt(City *city, s32 x, s32 y);
u8 getDistanceToWaterAt(City *city, s32 x, s32 y);

void saveTerrainTypes();
void remapTerrainTypes(City *city);

// Returns 0 if not found
u8 findTerrainTypeByName(String name);

