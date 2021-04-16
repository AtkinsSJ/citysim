#pragma once

struct TerrainDef
{
	String name;
	u8 typeID;
	
	String textAssetName;

	String spriteName;
	SpriteGroup *sprites;

	bool canBuildOn;
	
	Array<String> borderSpriteNames;

	bool drawBordersOver;
};

struct TerrainCatalogue
{
	OccupancyArray<TerrainDef> terrainDefs;

	HashTable<TerrainDef *> terrainDefsByName;
	StringTable terrainNames;

	HashTable<u8> terrainNameToOldType;
	HashTable<u8> terrainNameToType;
};

TerrainCatalogue terrainCatalogue = {};

struct TerrainLayer
{
	s32 terrainGenerationSeed;

	Array2<u8> tileTerrainType;
	Array2<u8> tileHeight;
	Array2<u8> tileDistanceToWater;

	Array2<u8> tileSpriteOffset;
	Array2<SpriteRef> tileSprite;
	Array2<SpriteRef> tileBorderSprite;
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

void assignTerrainTiles(City *city);

void saveTerrainTypes();
void remapTerrainTypes(City *city);

// Returns 0 if not found
u8 findTerrainTypeByName(String name);

