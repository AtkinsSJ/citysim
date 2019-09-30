#pragma once

enum TransportType
{
	Transport_Road,
	Transport_Rail,
	TransportTypeCount
};

struct TransportLayer
{
	DirtyRects dirtyRects;
	
	u8 *tileTransportTypes;

	u8 transportMaxDistance;
	u8 *tileTransportDistance[TransportTypeCount];
};

using Flags_TransportType = Flags<TransportType, u8>;

void initTransportLayer(TransportLayer *layer, City *city, MemoryArena *gameArena);
void updateTransportLayer(City *city, TransportLayer *layer);
void markTransportLayerDirty(TransportLayer *layer, Rect2I bounds);

void addTransportToTile(City *city, s32 x, s32 y, TransportType type);
void addTransportToTile(City *city, s32 x, s32 y, Flags_TransportType types);
bool doesTileHaveTransport(City *city, s32 x, s32 y, TransportType type);
bool doesTileHaveTransport(City *city, s32 x, s32 y, Flags_TransportType types);
s32 getDistanceToTransport(City *city, s32 x, s32 y, TransportType type);

void debugInspectTransport(WindowContext *context, City *city, s32 x, s32 y);
