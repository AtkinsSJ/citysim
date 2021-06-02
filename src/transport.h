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
	
	Array2<u8> tileTransportTypes;

	u8 transportMaxDistance;
	Array2<u8> tileTransportDistance[TransportTypeCount];
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

void debugInspectTransport(UI::Panel *panel, City *city, s32 x, s32 y);

void saveTransportLayer(TransportLayer *layer, struct BinaryFileWriter *writer);
bool loadTransportLayer(TransportLayer *layer, City *city, struct BinaryFileReader *reader);
