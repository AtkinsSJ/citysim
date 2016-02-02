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

inline bool isPathable(City *city, int32 x, int32 y)
{
	return tileExists(city, x, y)
		&& city->pathLayer.data[tileIndex(city, x, y)] > 0;
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
Coord pathToRectangle(City *city, Rect target, Coord from, MemoryArena *memoryArena)
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
#if 1
		struct PathingNode
		{
			bool initialised;
			Coord pos;
			int32 length;
			int32 heuristic;
			PathingNode *towardStart;

			PathingNode *next, *prev;
		};

		MemoryArena tempArena = beginTemporaryMemory(memoryArena);
		PathingNode *nodes = PushArray(&tempArena, PathingNode, city->width * city->height);
		PathingNode *startNode = nodes + tileIndex(city, from.x, from.y);
		*startNode = {true, from, 0, distance, 0, 0};
		PathingNode *openQueue = startNode;

		bool done = false;

		while (openQueue && !done)
		{
			// Pop best node off the open queue
			PathingNode *current = openQueue;
			openQueue = openQueue->next;

			// If we're at the target, exit
			if (current->heuristic <= 1)
			{
				// DONE!
				done = true;

				// For now we only care about the first step along the path, so
				// trace the path back to its beginning
				while (current->towardStart != startNode)
				{
					current = current->towardStart;
				}

				result = current->pos;
			}
			else
			{
				// Add all valid neighbours to the open queue
				Coord adjacents[] = {
					coord(current->pos.x-1, current->pos.y),
					coord(current->pos.x+1, current->pos.y),
					coord(current->pos.x, current->pos.y-1),
					coord(current->pos.x, current->pos.y+1),
				};

				for (int adjacentIndex=0; adjacentIndex<4; adjacentIndex++)
				{
					Coord newPos = adjacents[adjacentIndex];
					if (isPathable(city, newPos.x, newPos.y))
					{
						PathingNode *node = nodes + tileIndex(city, newPos.x, newPos.y);
						if (!node->initialised)
						{
							node->initialised = true;
							node->pos = newPos;
							node->length = current->length + 1;
							node->heuristic = manhattanDistance(target, node->pos);
							node->towardStart = current;
							node->next = 0;
							node->prev = 0;

							// Add to the open queue
							if (openQueue == null)
							{
								openQueue = node;
							}
							else
							{
								// Insert it before the first thing that's longer
								PathingNode *queuedNode = openQueue;
								while (queuedNode)
								{
									if ((queuedNode->length + queuedNode->heuristic)
										>= (node->length + node->heuristic))
									{
										// Insert before queuedNode
										if (queuedNode == openQueue)
										{
											openQueue = node;
										}
										else
										{
											node->prev = queuedNode->prev;
											queuedNode->prev->next = node;
										}
										node->next = queuedNode;
										queuedNode->prev = node;
										break;
									}
									else if (queuedNode->next == null)
									{
										queuedNode->next = node;
										node->prev = queuedNode;
										break;
									}
									else
									{
										queuedNode = queuedNode->next;
									}
								}
							}
						}
					}
				}
			}
		}

#else
		// For now, a beeline
		if (from.x < target.x) result.x = from.x + 1;
		else if (from.x >= (target.x + target.w)) result.x = from.x - 1;
		else if (from.y < target.y) result.y = from.y + 1;
		else if (from.y >= (target.y + target.h)) result.y = from.y - 1;
#endif
	}

	return result;
}