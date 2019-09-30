#pragma once

void initPollutionLayer(PollutionLayer *layer, City *city, MemoryArena *gameArena)
{
	s32 cityArea = areaOf(city->bounds);

	layer->tilePollution = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tilePollution, 0, cityArea);

	layer->tileBuildingContributions = allocateMultiple<s16>(gameArena, cityArea);
	fillMemory<s16>(layer->tileBuildingContributions, 0, cityArea);

	initDirtyRects(&layer->dirtyRects, gameArena, maxPollutionEffectDistance, city->bounds);
}

void updatePollutionLayer(City *city, PollutionLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	if (isDirty(&layer->dirtyRects))
	{
		// @Copypasta from updateLandValueLayer()
		{
			DEBUG_BLOCK_T("updatePollutionLayer: building effects", DCDT_Simulation);
			
			// Recalculate the building contributions
			for (auto rectIt = iterate(&layer->dirtyRects.rects);
				!rectIt.isDone;
				next(&rectIt))
			{
				Rect2I dirtyRect = getValue(rectIt);

				setRegion<s16>(layer->tileBuildingContributions, city->bounds.w, city->bounds.h, dirtyRect, 0);

				ChunkedArray<Building *> contributingBuildings = findBuildingsOverlappingArea(city, expand(dirtyRect, maxLandValueEffectDistance), 0);
				for (auto buildingIt = iterate(&contributingBuildings);
					!buildingIt.isDone;
					next(&buildingIt))
				{
					Building *building = getValue(buildingIt);
					BuildingDef *def = getBuildingDef(building);
					if (hasEffect(&def->pollutionEffect))
					{
						applyEffect(city, &def->pollutionEffect, centreOf(building->footprint), Effect_Add, layer->tileBuildingContributions, dirtyRect);
					}
				}

				// Now, clamp the tile values into the range we want!
				// The above process may have overflowed the -255 to 255 range we want the values to be,
				// but we don't clamp as we go along because then the end result might depend on the order that
				// buildings are processed.
				// eg, if we a total positive input of 1000 and a total negative of -900, the end result should be
				// +100, but if we did all the positives first and then all the negatives, we'd clamp at +255, and
				// then subtract the -900 and end up clamped to -255! (Or +255 if it happened the other way around.)
				// So, we instead give ourselves a lot of extra breathing room, and only clamp values at the very end.
				//
				// - Sam, 04/09/2019
				//
				for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
				{
					for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
					{
						s16 originalValue = getTileValue(city, layer->tileBuildingContributions, x, y);
						s16 newValue = clamp<s16>(originalValue, -255, 255);
						setTile(city, layer->tileBuildingContributions, x, y, newValue);
					}
				}
			}
		}

		// Recalculate overall value
		{
			DEBUG_BLOCK_T("updatePollutionLayer: combine", DCDT_Simulation);

			for (auto rectIt = iterate(&layer->dirtyRects.rects);
				!rectIt.isDone;
				next(&rectIt))
			{
				Rect2I dirtyRect = getValue(rectIt);

				for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
				{
					for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
					{
						// This is really lazy right now.
						// In future we might want to update things over time, but right now it's just
						// a glorified copy from the tileBuildingContributions array.

						s16 buildingContributions = getTileValue(city, layer->tileBuildingContributions, x, y);

						u8 newValue = (u8) clamp<s16>(buildingContributions, 0, 255);

						setTile(city, layer->tilePollution, x, y, newValue);
					}
				}
			}
		}

		clearDirtyRects(&layer->dirtyRects);
	}
}

void markPollutionLayerDirty(PollutionLayer *layer, Rect2I bounds)
{
	markRectAsDirty(&layer->dirtyRects, bounds);
}

void drawPollutionDataLayer(City *city, Rect2I visibleTileBounds)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	// @Copypasta drawLandValueDataLayer()
	// The TODO: @Speed: from that applies! This is very indirect.

	u8 *data = copyRegion(city->pollutionLayer.tilePollution, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);

	Array<V4> *palette = getPalette("pollution"_s);

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, (u16)palette->count, palette->items);
}

inline u8 getPollutionAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->pollutionLayer.tilePollution, x, y);
}

inline f32 getPollutionPercentAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->pollutionLayer.tilePollution, x, y) / 255.0f;
}
