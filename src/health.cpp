#pragma once

inline f32 getHealthCoveragePercentAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->healthLayer.tileHealthCoverage, x, y) * 0.01f;
}

void initHealthLayer(HealthLayer *layer, City *city, MemoryArena *gameArena)
{
	initSectorGrid(&layer->sectors, gameArena, city->bounds.w, city->bounds.h, 16, 8);

	initDirtyRects(&layer->dirtyRects, gameArena, maxLandValueEffectDistance, city->bounds);

	s32 cityArea = areaOf(city->bounds);
	layer->tileHealthCoverage = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileHealthCoverage, 0, cityArea);

	initChunkedArray(&layer->healthBuildings, &city->buildingRefsChunkPool);
}

void updateHealthLayer(City *city, HealthLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	if (isDirty(&layer->dirtyRects))
	{
		DEBUG_BLOCK_T("updateHealthLayer: dirty rects", DCDT_Simulation);

		// TODO: do we actually need dirty rects? I can't think of anything, unless we move the "register building" stuff to that.
		// for (auto rectIt = iterate(&layer->dirtyRects.rects);
		// 	!rectIt.isDone;
		// 	next(&rectIt))
		// {
		// 	Rect2I dirtyRect = getValue(rectIt);
		// }

		clearDirtyRects(&layer->dirtyRects);
	}

	{
		DEBUG_BLOCK_T("updateHealthLayer: sector updates", DCDT_Simulation);

		for (s32 i = 0; i < layer->sectors.sectorsToUpdatePerTick; i++)
		{
			BasicSector *sector = getNextSector(&layer->sectors);

			{
				DEBUG_BLOCK_T("updateHealthLayer: building health coverage", DCDT_Simulation);
				setRegion<u8>(layer->tileHealthCoverage, city->bounds.w, city->bounds.h, sector->bounds, 0);
				for (auto it = iterate(&layer->healthBuildings); hasNext(&it); next(&it))
				{
					Building *building = getBuilding(city, getValue(it));
					if (building != null)
					{
						BuildingDef *def = getBuildingDef(building);

						// TODO: Building effectiveness based on budget, overcrowding
						f32 effectiveness = 1.0f;

						if (!buildingHasPower(building))
						{
							effectiveness = 0.4f; // @Balance
						}

						applyEffect(city, &def->healthEffect, centreOf(building->footprint), Effect_Max, layer->tileHealthCoverage, sector->bounds, effectiveness);
					}
				}
			}
		}
	}
}

inline void markHealthLayerDirty(HealthLayer *layer, Rect2I bounds)
{
	markRectAsDirty(&layer->dirtyRects, bounds);
}

void drawHealthDataLayer(City *city, Rect2I visibleTileBounds)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	HealthLayer *layer = &city->healthLayer;

	// @Copypasta drawLandValueDataLayer() - the same TODO's apply!

	u8 *data = copyRegion(layer->tileHealthCoverage, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);

	Array<V4> *coveragePalette = getPalette("service_coverage"_s);

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, (u16)coveragePalette->count, coveragePalette->items);

	// Highlight buildings
	drawBuildingHighlights(city, &layer->healthBuildings);
	drawBuildingEffectRadii(city, &layer->healthBuildings, &BuildingDef::healthEffect);
}

void registerHealthBuilding(HealthLayer *layer, Building *building)
{
	append(&layer->healthBuildings, getReferenceTo(building));
}

void unregisterHealthBuilding(HealthLayer *layer, Building *building)
{
	bool success = findAndRemove(&layer->healthBuildings, getReferenceTo(building));
	ASSERT(success);
}
