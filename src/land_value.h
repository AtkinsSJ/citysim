#pragma once

struct LandValueLayer
{
	u8 *tileLandValue; // Cached total


};

void initLandValueLayer(LandValueLayer *layer, City *city, MemoryArena *gameArena);

void recalculateLandValue(City *city, Rect2I bounds);
void drawLandValueDataLayer(City *city, Rect2I visibleTileBounds);
u8 getLandValueAt(City *city, s32 x, s32 y);
