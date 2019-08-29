#pragma once

enum TransportType
{
	Transport_Road,
	Transport_Rail,
	TransportTypeCount
};

enum TransportTypeBits
{
	TransportBits_Road = 1 << Transport_Road,
	TransportBits_Rail = 1 << Transport_Rail,
};

struct TransportLayer
{
	DirtyRects dirtyRects;
	
	u8 *tileTransportTypes;

	u8 transportMaxDistance;
	u8 *tileTransportDistance[TransportTypeCount];
};

void initTransportLayer(TransportLayer *layer, City *city, MemoryArena *gameArena);
void updateTransportLayer(City *city, TransportLayer *layer);
void markTransportLayerDirty(TransportLayer *layer, Rect2I bounds);

void addTransportToTile(City *city, s32 x, s32 y, u8 transportTypes);
void removeAllTransportFromTile(City *city, s32 x, s32 y);
bool doesTileHaveTransport(City *city, s32 x, s32 y, u8 transportTypes);
s32 getDistanceToTransport(City *city, s32 x, s32 y, TransportType type);

// Deprecated
s32 calculateDistanceToRoad(City *city, s32 x, s32 y, s32 maxDistanceToCheck);
