#pragma once

void initFireLayer(FireLayer *layer, City *city, MemoryArena *gameArena)
{
	initSectorGrid(&layer->sectors, gameArena, city->bounds.w, city->bounds.h, 16);
	layer->nextSectorUpdateIndex = 0;
	layer->sectorsToUpdatePerTick = 8;

	initDirtyRects(&layer->dirtyRects, gameArena, maxLandValueEffectDistance, city->bounds);

	s32 cityArea = areaOf(city->bounds);

	layer->tileBuildingFireRisk = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileBuildingFireRisk, 0, cityArea);

	layer->tileOverallFireRisk = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileOverallFireRisk, 0, cityArea);

	initChunkedArray(&layer->fireProtectionBuildings, gameArena, 256);

	// Mark whole city as needing recalculating
	markRectAsDirty(&layer->dirtyRects, city->bounds);
}

inline void markFireLayerDirty(FireLayer *layer, Rect2I bounds)
{
	markRectAsDirty(&layer->dirtyRects, bounds);
}

void updateFireLayer(City *city, FireLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	if (isDirty(&layer->dirtyRects))
	{
		DEBUG_BLOCK_T("updateFireLayer: building effects", DCDT_Simulation);

		// Recalculate the building contributions
		for (auto rectIt = iterate(&layer->dirtyRects.rects);
			!rectIt.isDone;
			next(&rectIt))
		{
			Rect2I dirtyRect = getValue(rectIt);

			setRegion<u8>(layer->tileBuildingFireRisk, city->bounds.w, city->bounds.h, dirtyRect, 0);

			for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
			{
				for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
				{
					Building *building = getBuildingAt(city, x, y);
					if (building)
					{
						BuildingDef *def = getBuildingDef(building);
						u8 tileBuildingFireRisk = (u8)clamp(def->fireRisk * 100.0f, 0.0f, 255.0f);
						setTile(city, layer->tileBuildingFireRisk, x, y, tileBuildingFireRisk);
					}
				}
			}
		}

		clearDirtyRects(&layer->dirtyRects);
	}

	{
		DEBUG_BLOCK_T("updateFireLayer: overall calculation", DCDT_Simulation);

		for (s32 i = 0; i < layer->sectorsToUpdatePerTick; i++)
		{
			BasicSector *sector = &layer->sectors[layer->nextSectorUpdateIndex];

			for (s32 y = sector->bounds.y; y < sector->bounds.y + sector->bounds.h; y++)
			{
				for (s32 x = sector->bounds.x; x < sector->bounds.x + sector->bounds.w; x++)
				{
					//
					// TODO: Take other things into consideration once there *are* other things!
					//
					// (Maybe we don't even want to keep the tileBuildingFireRisk array at all? It's
					// kind of redundant because we can just query the building in the tile right here,
					// in an efficient way... This is placeholder so whatever!)
					//
					// - Sam, 12/09/2019
					//
					u8 buildingEffect = getTileValue(city, layer->tileBuildingFireRisk, x, y);

					setTile(city, layer->tileOverallFireRisk, x, y, buildingEffect);
				}
			}

			layer->nextSectorUpdateIndex = (layer->nextSectorUpdateIndex + 1) % getSectorCount(&layer->sectors);
		}
	}
}

void drawFireRiskDataLayer(City *city, Rect2I visibleTileBounds)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	FireLayer *layer = &city->fireLayer;

	// @Copypasta drawLandValueDataLayer() - the same TODO's apply!

	u8 *data = copyRegion(layer->tileOverallFireRisk, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);

	V4 colorMinFireRisk = color255(255, 255, 255, 128);
	V4 colorMaxFireRisk = color255(255,   0,   0, 128);

	V4 palette[256];
	f32 ratio = 1.0f / 255.0f;
	for (s32 i=0; i < 256; i++)
	{
		palette[i] = lerp(colorMinFireRisk, colorMaxFireRisk, i * ratio);
	}

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, 256, palette);

	// Highlight fire stations
	if (layer->fireProtectionBuildings.count > 0)
	{
		V4 highlightColor = color255(0,255,0,128);
		DrawRectsGroup *buildingHighlights = beginRectsGroupUntextured(&renderer->worldOverlayBuffer, renderer->shaderIds.untextured, layer->fireProtectionBuildings.count);
		for (auto it = iterate(&layer->fireProtectionBuildings); hasNext(&it); next(&it))
		{
			Building *building = getBuilding(city, getValue(it));
			// NB: We don't filter buildings outside of the visibleTileBounds because their radius might be
			// visible even if the building isn't!
			if (building != null)
			{
				addUntexturedRect(buildingHighlights, rect2(building->footprint), highlightColor);
			}
		}
		endRectsGroup(buildingHighlights);
	}
}

void registerFireProtectionBuilding(FireLayer *layer, Building *building)
{
	append(&layer->fireProtectionBuildings, getReferenceTo(building));
}

void unregisterFireProtectionBuilding(FireLayer *layer, Building *building)
{
	bool success = findAndRemove(&layer->fireProtectionBuildings, getReferenceTo(building));
	ASSERT(success);
}
