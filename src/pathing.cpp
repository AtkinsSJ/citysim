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

// Returns the next tile to walk to in order to path to 'target'
Coord pathToRectangle(Rect target, Coord position)
{
	Coord result = position;
	int32 distance = manhattanDistance(target, position);

	if (distance == 0)
	{
		// Already there
		result = position;
	}
	else if (distance == 1)
	{
		// Just walk right in
		if (position.x < target.x) result.x = position.x + 1;
		else if (position.x >= (target.x + target.w)) result.x = position.x - 1;
		else if (position.y < target.y) result.y = position.y + 1;
		else if (position.y >= (target.y + target.h)) result.y = position.y - 1;
	}
	else
	{
		// ACTUAL PATHFINDING!
		// For now, a beeline

		if (position.x < target.x) result.x = position.x + 1;
		else if (position.x >= (target.x + target.w)) result.x = position.x - 1;
		else if (position.y < target.y) result.y = position.y + 1;
		else if (position.y >= (target.y + target.h)) result.y = position.y - 1;
	}

	return result;
}