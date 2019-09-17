#pragma once

void initCrimeLayer(CrimeLayer *layer, City *city, MemoryArena *gameArena)
{
	initSectorGrid(&layer->sectors, gameArena, city->bounds.w, city->bounds.h, 16, 8);

	initDirtyRects(&layer->dirtyRects, gameArena, maxLandValueEffectDistance, city->bounds);

	s32 cityArea = areaOf(city->bounds);

	layer->tilePoliceCoverage = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tilePoliceCoverage, 0, cityArea);

	layer->totalJailCapacity = 0;
	layer->occupiedJailCapacity = 0;

	initChunkedArray(&layer->policeBuildings, &city->buildingRefsChunkPool);
}

void updateCrimeLayer(City *city, CrimeLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	if (isDirty(&layer->dirtyRects))
	{
		DEBUG_BLOCK_T("updateCrimeLayer: dirty rects", DCDT_Simulation);
		clearDirtyRects(&layer->dirtyRects);
	}

	// Recalculate jail capacity
	// NB: This only makes sense if we assume that building jail capacities can change while
	// the game is running. It'll only happen incredibly rarely during normal play, but during
	// development we have this whole hot-loaded building defs system, so it's better to be safe.
	layer->totalJailCapacity = 0;
	for (auto it = iterate(&layer->policeBuildings); hasNext(&it); next(&it))
	{
		Building *building = getBuilding(city, getValue(it));
		if (building != null)
		{
			BuildingDef *def = getBuildingDef(building);
			if (def->jailCapacity > 0)  layer->totalJailCapacity += def->jailCapacity;
		}
	}

	{
		DEBUG_BLOCK_T("updateCrimeLayer: sector updates", DCDT_Simulation);

		for (s32 i = 0; i < layer->sectors.sectorsToUpdatePerTick; i++)
		{
			BasicSector *sector = getNextSector(&layer->sectors);

			{
				DEBUG_BLOCK_T("updateCrimeLayer: building police coverage", DCDT_Simulation);
				setRegion<u8>(layer->tilePoliceCoverage, city->bounds.w, city->bounds.h, sector->bounds, 0);
				for (auto it = iterate(&layer->policeBuildings); hasNext(&it); next(&it))
				{
					Building *building = getBuilding(city, getValue(it));
					if (building != null)
					{
						BuildingDef *def = getBuildingDef(building);

						if (hasEffect(&def->policeEffect))
						{
							// TODO: Building effectiveness based on budget
							f32 effectiveness = 1.0f;

							if (!buildingHasPower(building))
							{
								effectiveness = 0.4f; // @Balance

								// TODO: Consider water access too
							}

							applyEffect(city, &def->policeEffect, centreOf(building->footprint), Effect_Add, layer->tilePoliceCoverage, sector->bounds, effectiveness);
						}
					}
				}
			}
		}
	}
}

void drawCrimeDataLayer(City *city, Rect2I visibleTileBounds)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	CrimeLayer *layer = &city->crimeLayer;

	// @Copypasta drawFireRiskDataLayer() - the same TODO's apply!

#if 0
	u8 *data = copyRegion(layer->tileOverallFireRisk, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);

	Array<V4> *palette = getPalette("risk"s);

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, (u16)palette->count, palette->items);
#else
	// Just draw the protection
	u8 *data = copyRegion(layer->tilePoliceCoverage, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);

	Array<V4> *palette = getPalette("service_coverage"s);

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, (u16)palette->count, palette->items);
#endif

	// Highlight fire stations
	if (layer->policeBuildings.count > 0)
	{
		Array<V4> *buildingsPalette = getPalette("service_buildings"s);
		s32 paletteIndexPowered   = 0;
		s32 paletteIndexUnpowered = 1;

		DrawRectsGroup *buildingHighlights = beginRectsGroupUntextured(&renderer->worldOverlayBuffer, renderer->shaderIds.untextured, layer->policeBuildings.count);
		for (auto it = iterate(&layer->policeBuildings); hasNext(&it); next(&it))
		{
			Building *building = getBuilding(city, getValue(it));
			// NB: We don't filter buildings outside of the visibleTileBounds because their radius might be
			// visible even if the building isn't!
			if (building != null)
			{
				s32 paletteIndex = (buildingHasPower(building) ? paletteIndexPowered : paletteIndexUnpowered);
				addUntexturedRect(buildingHighlights, rect2(building->footprint), (*buildingsPalette)[paletteIndex]);
			}
		}
		endRectsGroup(buildingHighlights);
	}
}

void registerPoliceBuilding(CrimeLayer *layer, Building *building)
{
	append(&layer->policeBuildings, getReferenceTo(building));
}

void unregisterPoliceBuilding(CrimeLayer *layer, Building *building)
{
	bool success = findAndRemove(&layer->policeBuildings, getReferenceTo(building));
	ASSERT(success);
}
