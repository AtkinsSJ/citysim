#pragma once

struct LandValueLayer
{
	DirtyRects dirtyRects;

	SectorGrid<BasicSector> sectors;
	s32 nextSectorUpdateIndex;
	s32 sectorsToUpdatePerTick;

	s16 *tileBuildingContributions;

	u8 *tileLandValue; // Cached total
};

const s32 maxLandValueEffectDistance = 16; // TODO: Better value for this!

void initLandValueLayer(LandValueLayer *layer, City *city, MemoryArena *gameArena);
void updateLandValueLayer(City *city, LandValueLayer *layer);
void markLandValueLayerDirty(LandValueLayer *layer, Rect2I bounds);

void drawLandValueDataLayer(City *city, Rect2I visibleTileBounds);
u8 getLandValueAt(City *city, s32 x, s32 y);
f32 getLandValuePercentAt(City *city, s32 x, s32 y);
