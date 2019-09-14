#pragma once

void initLandValueLayer(LandValueLayer *layer, City *city, MemoryArena *gameArena)
{
	initSectorGrid(&layer->sectors, gameArena, city->bounds.w, city->bounds.h, 16);
	layer->nextSectorUpdateIndex = 0;
	layer->sectorsToUpdatePerTick = 8;

	s32 cityArea = areaOf(city->bounds);

	layer->tileLandValue = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileLandValue, 0, cityArea);

	layer->tileBuildingContributions = allocateMultiple<s16>(gameArena, cityArea);
	fillMemory<s16>(layer->tileBuildingContributions, 0, cityArea);

	initDirtyRects(&layer->dirtyRects, gameArena, maxLandValueEffectDistance, city->bounds);

	// Mark whole city as needing recalculating
	markRectAsDirty(&layer->dirtyRects, city->bounds);
}

inline void markLandValueLayerDirty(LandValueLayer *layer, Rect2I bounds)
{
	markRectAsDirty(&layer->dirtyRects, bounds);
}

void updateLandValueLayer(City *city, LandValueLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	if (isDirty(&layer->dirtyRects))
	{
		{
			DEBUG_BLOCK_T("updateLandValueLayer: building effects", DCDT_Simulation);
			
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
					if (hasEffect(&def->landValueEffect))
					{
						applyEffect(city, &def->landValueEffect, centreOf(building->footprint), Effect_Add, layer->tileBuildingContributions, dirtyRect);
					}
				}

				//
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

		clearDirtyRects(&layer->dirtyRects);
	}

	// Recalculate overall value
	{
		DEBUG_BLOCK_T("updateLandValueLayer: overall calculation", DCDT_Simulation);

		for (s32 i = 0; i < layer->sectorsToUpdatePerTick; i++)
		{
			BasicSector *sector = &layer->sectors[layer->nextSectorUpdateIndex];

			for (s32 y = sector->bounds.y; y < sector->bounds.y + sector->bounds.h; y++)
			{
				for (s32 x = sector->bounds.x; x < sector->bounds.x + sector->bounds.w; x++)
				{
					// Right now, we have very little to base this on!
					// This explains how SC3K does it: http://www.sc3000.com/knowledge/showarticle.cfm?id=1132
					// (However, apparently SC3K has an overflow bug with land value, so ehhhhhh...)
					// -> Maybe we should use a larger int or even a float for the land value. Memory is cheap!
					//
					// Anyway... being near water, and being near forest are positive.
					// Pollution, crime, good service coverage, and building effects all play their part.
					// So does terrain height if we ever implement non-flat terrain!
					//
					// Also, global effects can influence land value. AKA, is the city nice to live in?
					//
					// At some point it'd probably be sensible to cache some of the factors in intermediate arrays,
					// eg the effects of buildings, or distance to water, so we don't have to do a search for them for each tile.

					f32 landValue = 0.1f;

					// Waterfront = valuable
					s32 distanceToWater = getTileValue(city, city->tileDistanceToWater, x, y);
					if (distanceToWater < 10)
					{
						landValue += (10 - distanceToWater) * 0.1f * 0.25f;
					}

					// Building effects
					f32 buildingEffect = getTileValue(city, layer->tileBuildingContributions, x, y) / 255.0f;
					landValue += buildingEffect;

					// Fire protection = good
					f32 fireProtection = getFireProtectionPercentAt(city, x, y);
					landValue += fireProtection * 0.1f;

					// Pollution = bad
					f32 pollutionEffect = getPollutionPercentAt(city, x, y) * 0.1f;
					landValue -= pollutionEffect;

					setTile(city, layer->tileLandValue, x, y, clamp01AndMap_u8(landValue));
				}
			}

			layer->nextSectorUpdateIndex = (layer->nextSectorUpdateIndex + 1) % getSectorCount(&layer->sectors);
		}
	}
}

void drawLandValueDataLayer(City *city, Rect2I visibleTileBounds)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	//
	// TODO: @Speed: Right now we're constructing an array that's just a copy of a section of the land value array,
	// and then passing that - when we could instead pass the whole array as a texture, and only draw the
	// visible part of it. (Or even draw the whole thing.)
	// We could pass the array as a texture, and the palette as a second texture, and then use a shader
	// similar to the one I developed for GLunatic.
	// But for right now, keep things simple.
	//
	// -Sam, 04/09/2019
	//
	// (NB: This code is copypasta'd in several places, so when we fix this, fix those too!)
	//

	u8 *data = copyRegion(city->landValueLayer.tileLandValue, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);

	Array<V4> *palette = getPalette(makeString("land_value"));

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, (u16)palette->count, palette->items);
}

inline f32 getLandValuePercentAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->landValueLayer.tileLandValue, x, y) / 255.0f;
}
