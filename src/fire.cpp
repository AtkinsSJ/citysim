#pragma once

void initFireLayer(FireLayer *layer, City *city, MemoryArena *gameArena)
{
	initSectorGrid(&layer->sectors, gameArena, city->bounds.w, city->bounds.h, 16, 8);

	initDirtyRects(&layer->dirtyRects, gameArena, maxLandValueEffectDistance, city->bounds);

	s32 cityArea = areaOf(city->bounds);

	layer->tileBuildingFireRisk = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileBuildingFireRisk, 0, cityArea);
	layer->tileTotalFireRisk = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileTotalFireRisk, 0, cityArea);
	layer->tileFireProtection = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileFireProtection, 0, cityArea);
	layer->tileOverallFireRisk = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileOverallFireRisk, 0, cityArea);

	initChunkedArray(&layer->fireProtectionBuildings, &city->buildingRefsChunkPool);
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

			// Building fire risk
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

		for (s32 i = 0; i < layer->sectors.sectorsToUpdatePerTick; i++)
		{
			BasicSector *sector = getNextSector(&layer->sectors);

			{
				DEBUG_BLOCK_T("updateFireLayer: building fire protection", DCDT_Simulation);
				// Building fire protection
				setRegion<u8>(layer->tileFireProtection, city->bounds.w, city->bounds.h, sector->bounds, 0);
				for (auto it = iterate(&layer->fireProtectionBuildings); hasNext(&it); next(&it))
				{
					Building *building = getBuilding(city, getValue(it));
					if (building != null)
					{
						BuildingDef *def = getBuildingDef(building);

						// TODO: Building effectiveness based on budget
						f32 effectiveness = 1.0f;

						if (!buildingHasPower(building))
						{
							// NB: We might want to instead reduce the effectiveness to 1/4 or something,
							// but that's a balance issue
							effectiveness = 0.0f;
						}

						applyEffect(city, &def->fireProtection, centreOf(building->footprint), Effect_Max, layer->tileFireProtection, sector->bounds, effectiveness);
					}
				}
			}

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
					u8 totalRisk = buildingEffect;
					setTile(city, layer->tileTotalFireRisk, x, y, totalRisk);

					f32 protectionPercent = getTileValue(city, layer->tileFireProtection, x, y) * 0.01f;
					u8 result = lerp<u8>(totalRisk, 0, protectionPercent);
					setTile(city, layer->tileOverallFireRisk, x, y, result);
				}
			}
		}
	}
}

void drawFireRiskDataLayer(City *city, Rect2I visibleTileBounds)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	FireLayer *layer = &city->fireLayer;

	// @Copypasta drawLandValueDataLayer() - the same TODO's apply!

#if 1
	u8 *data = copyRegion(layer->tileOverallFireRisk, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);

	Array<V4> *palette = getPalette("risk"s);

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, (u16)palette->count, palette->items);
#else
	// Just draw the protection
	u8 *data = copyRegion(layer->tileFireProtection, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);

	Array<V4> *palette = getPalette("service_coverage"s);

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, (u16)palette->count, palette->items);
#endif

	// Highlight fire stations
	if (layer->fireProtectionBuildings.count > 0)
	{
		Array<V4> *buildingsPalette = getPalette("service_buildings"s);
		s32 paletteIndexPowered   = 0;
		s32 paletteIndexUnpowered = 1;

		DrawRectsGroup *buildingHighlights = beginRectsGroupUntextured(&renderer->worldOverlayBuffer, renderer->shaderIds.untextured, layer->fireProtectionBuildings.count);
		for (auto it = iterate(&layer->fireProtectionBuildings); hasNext(&it); next(&it))
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

void registerFireProtectionBuilding(FireLayer *layer, Building *building)
{
	append(&layer->fireProtectionBuildings, getReferenceTo(building));
}

void unregisterFireProtectionBuilding(FireLayer *layer, Building *building)
{
	bool success = findAndRemove(&layer->fireProtectionBuildings, getReferenceTo(building));
	ASSERT(success);
}

u8 getFireRiskAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->fireLayer.tileTotalFireRisk, x, y);
}

f32 getFireProtectionPercentAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->fireLayer.tileFireProtection, x, y) * 0.01f;
}
