void floodFillPathingConnectivity(City *city, int32 x, int32 y, int32 fillValue)
{
	city->pathLayer.data[tileIndex(city, x, y)] = fillValue;

	if (tileExists(city, x-1, y)
	&& (city->pathLayer.data[tileIndex(city, x-1, y)] == -1))
	{
		floodFillPathingConnectivity(city, x-1, y, fillValue);
	}
	
	if (tileExists(city, x+1, y)
	&& (city->pathLayer.data[tileIndex(city, x+1, y)] == -1))
	{
		floodFillPathingConnectivity(city, x+1, y, fillValue);
	}

	if (tileExists(city, x, y+1)
	&& (city->pathLayer.data[tileIndex(city, x, y+1)] == -1))
	{
		floodFillPathingConnectivity(city, x, y+1, fillValue);
	}

	if (tileExists(city, x, y-1)
	&& (city->pathLayer.data[tileIndex(city, x, y-1)] == -1))
	{
		floodFillPathingConnectivity(city, x, y-1, fillValue);
	}
}

void recalculatePathingConnectivity(City *city)
{
	// This is a flood fill.
	// First, normalise things so path tiles are -1, others are 0
	// Then, iterate over the tiles and flood fill from each -1 value.

	// Reset things to 0/-1
	int32 maxTileIndex = city->width * city->height;
	for (int32 tileIndex = 0; tileIndex < maxTileIndex; ++tileIndex)
	{
		if (city->pathLayer.data[tileIndex] > 0)
		{
			city->pathLayer.data[tileIndex] = -1;
		}
	}

	int32 fillValue = 1;

	// Find -1 tiles
	for (int32 y=0, tileIndex = 0; y<city->height; y++)
	{
		for (int32 x=0; x<city->width; x++, tileIndex++)
		{
			if (city->pathLayer.data[tileIndex] == -1)
			{
				// Flood fill from here!
				floodFillPathingConnectivity(city, x, y, fillValue);

				fillValue++;
			}
		}
	}
}

bool canPathTo(City *city, Rect target, Coord from)
{
	bool result = false;

	int32 pathGroup = city->pathLayer.data[tileIndex(city, from.x, from.y)];
	if (pathGroup > 0)
	{
		// Test each tile around the edge of the target rect for connectivity

		// Horizontals
		for (int x=target.x; x<target.x + target.w; x++)
		{
			if (pathGroup == city->pathLayer.data[tileIndex(city, x, target.y-1)])
			{
				result = true;
				break;
			}
			if (pathGroup == city->pathLayer.data[tileIndex(city, x, target.y + target.h)])
			{
				result = true;
				break;
			}
		}

		// Only test verticals if no path has yet been found
		if (!result)
		{
			for (int y=target.y; y<target.y + target.h; y++)
			{
				if (pathGroup == city->pathLayer.data[tileIndex(city, target.x-1, y)])
				{
					result = true;
					break;
				}
				if (pathGroup == city->pathLayer.data[tileIndex(city, target.x + target.w, y)])
				{
					result = true;
					break;
				}
			}
		}
	}

	return result;
}

// Returns the next tile to walk to in order to path to 'target'
Coord pathToRectangle(City *city, Rect target, Coord from)
{
	Coord result = from;
	int32 distance = manhattanDistance(target, from);

	if (distance == 0)
	{
		// Already there
		result = from;
	}
	else if (distance == 1)
	{
		// Just walk right in
		if (from.x < target.x) result.x = from.x + 1;
		else if (from.x >= (target.x + target.w)) result.x = from.x - 1;
		else if (from.y < target.y) result.y = from.y + 1;
		else if (from.y >= (target.y + target.h)) result.y = from.y - 1;
	}
	else
	{
		// ACTUAL PATHFINDING!
		// For now, a beeline

		if (from.x < target.x) result.x = from.x + 1;
		else if (from.x >= (target.x + target.w)) result.x = from.x - 1;
		else if (from.y < target.y) result.y = from.y + 1;
		else if (from.y >= (target.y + target.h)) result.y = from.y - 1;
	}

	return result;
}