#pragma once

struct PollutionLayer
{
	DirtyRects dirtyRects;
	
	s16 *tileBuildingContributions;

	u8 *tilePollution; // Cached total
};

const s32 maxPollutionEffectDistance = 16; // TODO: Better value for this!

void initPollutionLayer(PollutionLayer *layer, City *city, MemoryArena *gameArena);
void updatePollutionLayer(City *city, PollutionLayer *layer);
void markPollutionLayerDirty(PollutionLayer *layer, Rect2I bounds);

void drawPollutionDataView(City *city, Rect2I visibleTileBounds);
u8 getPollutionAt(City *city, s32 x, s32 y);
f32 getPollutionPercentAt(City *city, s32 x, s32 y);
