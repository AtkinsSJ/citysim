#pragma once

void initLandValueLayer(LandValueLayer *layer, City *city, MemoryArena *gameArena)
{
	s32 cityArea = areaOf(city->bounds);

	layer->tileLandValue = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileLandValue, 128, cityArea);
}

void recalculateLandValue(City *city, Rect2I bounds)
{
	LandValueLayer *layer = &city->landValueLayer;

	for (s32 y = bounds.y; y < bounds.y + bounds.h; y++)
	{
		for (s32 x = bounds.x; x < bounds.x + bounds.w; x++)
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
			s32 waterTerrainType = findTerrainTypeByName(makeString("Water"));
			s32 distanceToWater = calculateDistanceTo(city, x, y, 10, [&](City *city, s32 x, s32 y) {
				return getTerrainAt(city, x, y)->type == waterTerrainType;
			});
			if (distanceToWater < 10)
			{
				landValue += (10 - distanceToWater) * 0.1f * 0.25f;
			}

			// Forest = valuable
			// TODO: We'd probably rather put all building yimby/nimby effects in a separate array that's updated
			// as buildings are constructed/demolished, because trying to look outwards to find any builidngs
			// that could be influencing this tile is going to involve a lot of busywork!

			 

			setTile(city, layer->tileLandValue, x, y, clamp01AndMap(landValue));
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

	u8 *data = copyRegion(city->landValueLayer.tileLandValue, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);


	// TODO: Palette assets! Don't just recalculate this every time, that's ridiculous!

	V4 colorMinLandValue = color255(255, 255, 255, 128);
	V4 colorMaxLandValue = color255(  0,   0, 255, 128);

	V4 palette[256];
	f32 ratio = 1.0f / 255.0f;
	for (s32 i=0; i < 256; i++)
	{
		palette[i] = lerp(colorMinLandValue, colorMaxLandValue, i * ratio);
	}

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, 256, palette);
}

inline u8 getLandValueAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->landValueLayer.tileLandValue, x, y);
}
