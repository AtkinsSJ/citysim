#pragma once

struct FireLayer
{
	DirtyRects dirtyRects;

	u8 *tileBuildingFireRisk;
};

void initFireLayer(FireLayer *layer, City *city, MemoryArena *gameArena);
void updateFireLayer(City *city, FireLayer *layer);
void markFireLayerDirty(FireLayer *layer, Rect2I bounds);
void drawFireRiskDataLayer(City *city, Rect2I visibleTileBounds);
