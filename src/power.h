#pragma once

struct PowerGroup
{
	s32 production;
	s32 consumption;

	// TODO: @Size These are always either 1-wide or 1-tall, and up to SECTOR_SIZE in the other direction, so we could use a much smaller struct than Rect2I!
	ChunkedArray<Rect2I> sectorBoundaries; // Places in nighbouring sectors that are adjacent to this PowerGroup
	s32 networkID;
};

struct PowerSector
{
	Sector *base;

	// 0 = none, >0 = any tile with the same value is connected
	// POWER_GROUP_UNKNOWN is used as a temporary value while recalculating
	u8 tilePowerGroup[SECTOR_SIZE][SECTOR_SIZE];

	// NB: Power groups start at 1, (0 means "none") so subtract 1 from the value in tilePowerGroup to get the index!
	ChunkedArray<PowerGroup> powerGroups;
};

struct PowerNetwork
{
	s32 id;

	ChunkedArray<PowerGroup *> groups;

	s32 cachedProduction;
	s32 cachedConsumption;
};

struct PowerLayer
{
	bool isDirty;

	// We probably don't want these here, we probably want some kind of templatey thing that gives us sectors.
	s32 sectorsX, sectorsY;
	s32 sectorCount;
	PowerSector *sectors;

	ChunkedArray<PowerNetwork> networks;

	ChunkPool<PowerGroup> powerGroupsChunkPool;
	ChunkPool<PowerGroup *> powerGroupPointersChunkPool;
	
	s32 cachedCombinedProduction;
	s32 cachedCombinedConsumption;
};

const u8 POWER_GROUP_UNKNOWN = 255;

// Public API
void initPowerLayer(PowerLayer *layer, City *city, MemoryArena *gameArena);
void updatePowerLayer(City *city, PowerLayer *layer);
void markPowerLayerDirty(PowerLayer *layer, Rect2I area);
PowerNetwork *getPowerNetworkAt(PowerLayer *powerLayer, s32 x, s32 y);


// Private-but-actually-still-accessible API
PowerNetwork *newPowerNetwork(PowerLayer *layer);
void freePowerNetwork(PowerNetwork *network);

void updateSectorPowerValues(PowerSector *sector);

void floodFillSectorPowerGroup(PowerSector *sector, s32 x, s32 y, u8 fillValue);
void setRectPowerGroupUnknown(PowerSector *sector, Rect2I area);
void recalculateSectorPowerGroups(City *city, PowerSector *sector);
void floodFillCityPowerNetwork(PowerLayer *powerLayer, PowerGroup *powerGroup, PowerNetwork *network);
void recalculatePowerConnectivity(PowerLayer *powerLayer);
