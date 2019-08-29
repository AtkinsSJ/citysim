#pragma once

void initTransportLayer(TransportLayer *layer, City *city, MemoryArena *gameArena)
{
	initDirtyRects(&layer->dirtyRects, gameArena);

	s32 cityArea = city->width * city->height;
	layer->tileTransportTypes = allocateMultiple<u8>(gameArena, cityArea);

	layer->transportMaxDistance = 8;

	for (s32 type = 0; type < TransportTypeCount; type++)
	{
		layer->tileTransportDistance[type] = allocateMultiple<u8>(gameArena, cityArea);
		fillMemory<u8>(layer->tileTransportDistance[type], 255, cityArea);
	}
}

void updateTransportLayer(City *city, TransportLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	if (isDirty(&layer->dirtyRects))
	{
		// Calculate transport types on each tile
		// So, I have two ideas about this:
		// 1: memset the area to 0 and then iterate through all the buildings in that area, applying their transport types
		// or 2: go tile-by-tile within the area, and see if a building exists or not, and set the transport types from that.
		// I'm not sure which is better? Right now a memset is easy because we're doing the entire map, but
		// that won't be the case for long and memsetting a rectangle within the city is going to be way fiddlier.
		// Also, there's a performance cost to gathering all the buildings up in that area into an array for iterating.
		// So, I think #2 is the better option, but I should test that later if it becomes expensive performance-wise.
		// - Sam, 28/08/2019

		for (auto it = iterate(&layer->dirtyRects.rects);
			!it.isDone;
			next(&it))
		{
			Rect2I dirtyRect = getValue(it);

			// Transport types on tile, based on what buildings are present
			for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
			{
				for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
				{
					Building *building = getBuildingAt(city, x, y);
					if (building != null)
					{
						BuildingDef *def = getBuildingDef(building->typeID);
						setTile<u8>(city, layer->tileTransportTypes, x, y, def->transportTypes);
					}
					else
					{
						setTile<u8>(city, layer->tileTransportTypes, x, y, 0);
					}
				}
			}

			// Clear the surrounding "distance to road" stuff from the rectangle
			Rect2I distanceChangedRect = intersect(expand(dirtyRect, layer->transportMaxDistance), irectXYWH(0,0,city->width, city->height));
			*get(it) = distanceChangedRect; // NB: We do this here so we don't have to do expand() over and over again below.

			for (s32 y = distanceChangedRect.y; y < distanceChangedRect.y + distanceChangedRect.h; y++)
			{
				for (s32 x = distanceChangedRect.x; x < distanceChangedRect.x + distanceChangedRect.w; x++)
				{
					for (s32 type = 0; type < TransportTypeCount; type++)
					{
						u8 distance = doesTileHaveTransport(city, x, y, 1 << type) ? 0 : 255;
						setTile<u8>(city, layer->tileTransportDistance[type], x, y, distance);
					}
				}
			}
		}

		// Transport distance recalculation
		// The simplest possible algorithm is, just spread the 0s out that we marked above.
		// (If a tile is not 0, set it to the min() of its 8 neighbours, plus 1.)
		// We have to iterate through the area `transportMaxDistance+1` times, but it should be fast enough probably.
		for (s32 iteration = 0; iteration < layer->transportMaxDistance; iteration++)
		{
			for (auto it = iterate(&layer->dirtyRects.rects);
				!it.isDone;
				next(&it))
			{
				// NB: The rects are already expanded, see above.
				Rect2I dirtyRect = getValue(it);

				for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
				{
					for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
					{
						for (s32 type = 0; type < TransportTypeCount; type++)
						{
							if (getTileValue(city, layer->tileTransportDistance[type], x, y) != 0)
							{
								u8 minDistance = min({
									getTileValueIfExists<u8>(city, layer->tileTransportDistance[type], x-1, y-1, 255),
									getTileValueIfExists<u8>(city, layer->tileTransportDistance[type], x  , y-1, 255),
									getTileValueIfExists<u8>(city, layer->tileTransportDistance[type], x+1, y-1, 255),
									getTileValueIfExists<u8>(city, layer->tileTransportDistance[type], x-1, y  , 255),
								//	getTileValueIfExists<u8>(city, layer->tileTransportDistance[type], x  , y  , 255),
									getTileValueIfExists<u8>(city, layer->tileTransportDistance[type], x+1, y  , 255),
									getTileValueIfExists<u8>(city, layer->tileTransportDistance[type], x-1, y+1, 255),
									getTileValueIfExists<u8>(city, layer->tileTransportDistance[type], x  , y+1, 255),
									getTileValueIfExists<u8>(city, layer->tileTransportDistance[type], x+1, y+1, 255),
								});

								if (minDistance != 255)  minDistance++;
								if (minDistance > layer->transportMaxDistance)  minDistance = 255;

								setTile<u8>(city, layer->tileTransportDistance[type], x, y, minDistance);
							}
						}
					}
				}
			}
		}

		clearDirtyRects(&layer->dirtyRects);
	}
}

void markTransportLayerDirty(TransportLayer *layer, Rect2I bounds)
{
	markRectAsDirty(&layer->dirtyRects, bounds);
}

bool doesTileHaveTransport(City *city, s32 x, s32 y, u8 transportTypes)
{
	bool result = false;

	if (tileExists(city, x, y))
	{
		u8 transportTypesAtTile = getTileValue(city, city->transportLayer.tileTransportTypes, x, y);
		result = (transportTypes & transportTypesAtTile) != 0;
	}

	return result;
}

void addTransportToTile(City *city, s32 x, s32 y, u8 transportTypes)
{
	u8 oldValue = getTileValue(city, city->transportLayer.tileTransportTypes, x, y);
	u8 newValue = oldValue | transportTypes;
	setTile(city, city->transportLayer.tileTransportTypes, x, y, newValue);
}

void removeAllTransportFromTile(City *city, s32 x, s32 y)
{
	setTile(city, city->transportLayer.tileTransportTypes, x, y, (u8)0);
}

s32 getDistanceToTransport(City *city, s32 x, s32 y, TransportType type)
{
	return getTileValue(city, city->transportLayer.tileTransportDistance[type], x, y);
}

/**
 * Distance to road, counting diagonal distances as 1.
 * If nothing is found within the maxDistanceToCheck, returns a *really big number*.
 * If the tile itself is a road, just returns 0 as you'd expect.
 * DEPRECATED
 */
s32 calculateDistanceToRoad(City *city, s32 x, s32 y, s32 maxDistanceToCheck)
{
	DEBUG_FUNCTION();
	
	s32 result = s32Max;

	if (doesTileHaveTransport(city, x, y, TransportBits_Road))
	{
		result = 0;
	}
	else
	{
		bool done = false;

		for (s32 distance = 1;
			 !done && distance <= maxDistanceToCheck;
			 distance++)
		{
			s32 leftX   = x - distance;
			s32 rightX  = x + distance;
			s32 bottomY = y - distance;
			s32 topY    = y + distance;

			for (s32 px = leftX; px <= rightX; px++)
			{
				if (doesTileHaveTransport(city, px, bottomY, TransportBits_Road))
				{
					result = distance;
					done = true;
					break;
				}

				if (doesTileHaveTransport(city, px, topY, TransportBits_Road))
				{
					result = distance;
					done = true;
					break;
				}
			}

			if (done) break;

			for (s32 py = bottomY; py <= topY; py++)
			{
				if (doesTileHaveTransport(city, leftX, py, TransportBits_Road))
				{
					result = distance;
					done = true;
					break;
				}

				if (doesTileHaveTransport(city, rightX, py, TransportBits_Road))
				{
					result = distance;
					done = true;
					break;
				}
			}
		}
	}

	return result;
}
