#pragma once

void initTransportLayer(TransportLayer *layer, City *city, MemoryArena *gameArena)
{
	layer->tileTransportTypes = allocateMultiple<u8>(gameArena, city->width * city->height);
	layer->isDirty = false;
	layer->dirtyRect = irectXYWH(0,0,0,0);
}

void updateTransportLayer(City *city, TransportLayer *layer)
{
	if (layer->isDirty)
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

		for (s32 y = layer->dirtyRect.y; y < layer->dirtyRect.y + layer->dirtyRect.h; y++)
		{
			for (s32 x = layer->dirtyRect.x; x < layer->dirtyRect.x + layer->dirtyRect.w; x++)
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

		// TODO: transport distance recalculation

		layer->isDirty = false;
	}
}

void markTransportLayerDirty(TransportLayer *layer, Rect2I bounds)
{
	if (layer->isDirty)
	{
		// TODO: Probably more optimal to store a list of dirty rects rather than doing this?
		layer->dirtyRect = unionOf(layer->dirtyRect, bounds);
	}
	else
	{
		layer->dirtyRect = bounds;
		layer->isDirty = true;
	}
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

/**
 * Distance to road, counting diagonal distances as 1.
 * If nothing is found within the maxDistanceToCheck, returns a *really big number*.
 * If the tile itself is a road, just returns 0 as you'd expect.
 */
s32 calculateDistanceToRoad(City *city, s32 x, s32 y, s32 maxDistanceToCheck)
{
	DEBUG_FUNCTION();
	
	s32 result = s32Max;

	if (doesTileHaveTransport(city, x, y, Transport_Road))
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
				if (doesTileHaveTransport(city, px, bottomY, Transport_Road))
				{
					result = distance;
					done = true;
					break;
				}

				if (doesTileHaveTransport(city, px, topY, Transport_Road))
				{
					result = distance;
					done = true;
					break;
				}
			}

			if (done) break;

			for (s32 py = bottomY; py <= topY; py++)
			{
				if (doesTileHaveTransport(city, leftX, py, Transport_Road))
				{
					result = distance;
					done = true;
					break;
				}

				if (doesTileHaveTransport(city, rightX, py, Transport_Road))
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
