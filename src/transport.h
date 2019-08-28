#pragma once

enum TransportType
{
	Transport_Road = 1 << 0,
	Transport_Rail = 1 << 1,
};

struct TransportLayer
{
	bool isDirty;
	Rect2I dirtyRect;
	
	u8 *tileTransportTypes;

	// TODO: Cache the distance of each tile from a road/rail etc. with a limit of eg 8 tiles, so anything > 8 is 255, to make calculations faster and affect a limited area.
};

void initTransportLayer(TransportLayer *layer, City *city, MemoryArena *gameArena);
void updateTransportLayer(City *city, TransportLayer *layer);
void markTransportLayerDirty(TransportLayer *layer, Rect2I bounds);

void addTransportToTile(City *city, s32 x, s32 y, u8 transportTypes);
void removeAllTransportFromTile(City *city, s32 x, s32 y);
bool doesTileHaveTransport(City *city, s32 x, s32 y, u8 transportTypes);
s32 calculateDistanceToRoad(City *city, s32 x, s32 y, s32 maxDistanceToCheck);
