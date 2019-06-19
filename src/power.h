#pragma once

struct PowerGroup
{
	s32 production;
	s32 consumption;

	// TODO: @Size These are always either 1-wide or 1-tall, and up to SECTOR_SIZE in the other direction, so we could use a much smaller struct than Rect2I!
	ChunkedArray<Rect2I> sectorBoundaries; // Places in nighbouring sectors that are adjacent to this PowerGroup
	s32 networkID;
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
	
	s32 cachedCombinedProduction;
	s32 cachedCombinedConsumption;

	ChunkedArray<PowerNetwork> networks;

	ChunkPool<PowerGroup> powerGroupsChunkPool;
	ChunkPool<PowerGroup *> powerGroupPointersChunkPool;
};

const u8 POWER_GROUP_UNKNOWN = 255;

void initPowerLayer(MemoryArena *gameArena, PowerLayer *layer);
void updatePowerLayer(struct City *city, PowerLayer *layer);
void markPowerLayerDirty(PowerLayer *layer, Rect2I area);
