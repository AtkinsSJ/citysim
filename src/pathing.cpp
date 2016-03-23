
inline int32 pathGroupAt(City *city, int32 x, int32 y)
{
	int32 result = 0;

	if (tileExists(city, x, y))
	{
		result = city->pathLayer.data[tileIndex(city, x, y)];
	}

	return result;
}

inline bool isPathable(City *city, int32 x, int32 y)
{
	return pathGroupAt(city, x, y) > 0;
}

void floodFillPathingConnectivity(City *city, int32 x, int32 y, int32 fillValue)
{
	city->pathLayer.data[tileIndex(city, x, y)] = fillValue;

	if (pathGroupAt(city, x-1, y) == -1)
	{
		floodFillPathingConnectivity(city, x-1, y, fillValue);
	}
	
	if (pathGroupAt(city, x+1, y) == -1)
	{
		floodFillPathingConnectivity(city, x+1, y, fillValue);
	}

	if (pathGroupAt(city, x, y+1) == -1)
	{
		floodFillPathingConnectivity(city, x, y+1, fillValue);
	}

	if (pathGroupAt(city, x, y-1) == -1)
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

	city->pathLayer.pathGroupCount = 0;

	// Find -1 tiles
	for (int32 y=0, tileIndex = 0; y<city->height; y++)
	{
		for (int32 x=0; x<city->width; x++, tileIndex++)
		{
			if (city->pathLayer.data[tileIndex] == -1)
			{
				// Flood fill from here!
				floodFillPathingConnectivity(city, x, y, ++city->pathLayer.pathGroupCount);
			}
		}
	}
}

inline bool _findPathGroup(int32 *pathGroupCount, int32 **pathGroups, int32 pathGroup)
{
	bool found = false;
	for (int i=0; i< (*pathGroupCount); i++)
	{
		if (pathGroup == (*pathGroups)[i])
		{
			found = true;
			break;
		}
	}
	return found;
}

inline void _addPathGroup(int32 *pathGroupCount, int32 **pathGroups, int32 pathGroup)
{
	if (pathGroup)
	{
		// Try to add it to the array

		if (!_findPathGroup(pathGroupCount, pathGroups, pathGroup))
		{
			(*pathGroups)[(*pathGroupCount)++] = pathGroup;
		}
	}
}

bool canPathTo(City *city, Rect target, Coord from, MemoryArena *memoryArena)
{
	bool result = false;

	TemporaryMemoryArena tempArena = beginTemporaryMemory(memoryArena);

	// First, determine all path groups that are adjacent to the buiding 'from' is in, if any.
	int32 pathGroupCount = 0;
	int32 *pathGroups = PushArray(&tempArena, int32, city->pathLayer.pathGroupCount);
	Building *building = getBuildingAtPosition(city, from);
	if (building
		&& !buildingDefinitions[building->archetype].isPath)
	{
		// Find all adjacent path groups
		Rect fromRect = building->footprint;

		// Horizontals
		for (int x=fromRect.x; x<fromRect.x + fromRect.w; x++)
		{
			_addPathGroup(&pathGroupCount, &pathGroups, pathGroupAt(city, x, fromRect.y-1));
			_addPathGroup(&pathGroupCount, &pathGroups, pathGroupAt(city, x, fromRect.y + fromRect.h));
		}
		// Verticals
		for (int y=fromRect.y; y<fromRect.y + fromRect.h; y++)
		{
			_addPathGroup(&pathGroupCount, &pathGroups, pathGroupAt(city, fromRect.x-1, y));
			_addPathGroup(&pathGroupCount, &pathGroups, pathGroupAt(city, fromRect.x + fromRect.w, y));
		}
	}
	else
	{
		// Right now, paths are only created by buildings, but we might as well
		// handle non-buildings too.
		int32 pathGroup = pathGroupAt(city, from.x, from.y);
		if (pathGroup)
		{
			pathGroupCount = 1;
			pathGroups[0] = pathGroup;
		}
	}

	if (pathGroupCount > 0)
	{
		// Test each tile around the edge of the target rect for connectivity

		// Horizontals
		for (int x=target.x; x<target.x + target.w; x++)
		{
			if (_findPathGroup(&pathGroupCount, &pathGroups, pathGroupAt(city, x, target.y-1))
			 || _findPathGroup(&pathGroupCount, &pathGroups, pathGroupAt(city, x, target.y + target.h)))
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

				if (_findPathGroup(&pathGroupCount, &pathGroups, pathGroupAt(city, target.x-1, y))
				 || _findPathGroup(&pathGroupCount, &pathGroups, pathGroupAt(city, target.x + target.w, y)))
				{
					result = true;
					break;
				}
			}
		}
	}

	endTemporaryMemory(&tempArena);

	return result;
}

struct PathingNode
{
	bool initialised;
	Coord pos;
	int32 length;
	int32 heuristic;
	PathingNode *towardStart;

	PathingNode *next, *prev;
};

void _addPathNodeToQueue(City *city, PathingNode *nodes, Coord pos, PathingNode *parentNode, PathingNode **openQueue, Rect target)
{
	if (isPathable(city, pos.x, pos.y))
	{
		PathingNode *node = nodes + tileIndex(city, pos.x, pos.y);

		if (node->initialised)
		{
			// If this new route is faster, replace the old one.
			if (node->length > (parentNode->length + 1))
			{
				node->length = parentNode->length + 1;
				node->towardStart = parentNode;
			}
		}
		else
		{
			node->initialised = true;
			node->pos = pos;
			node->length = parentNode->length + 1;
			node->heuristic = manhattanDistance(target, node->pos);
			node->towardStart = parentNode;
			node->next = 0;
			node->prev = 0;

			// Add to the open queue
			if ((*openQueue) == null)
			{
				(*openQueue) = node;
			}
			else
			{
				// Insert it before the first thing that's longer
				PathingNode *queuedNode = (*openQueue);
				while (queuedNode)
				{
					if ((queuedNode->length + queuedNode->heuristic)
						>= (node->length + node->heuristic))
					{
						// Insert before queuedNode
						if (queuedNode == (*openQueue))
						{
							(*openQueue) = node;
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

// Returns the next tile to walk to in order to path to 'target'
Coord pathToRectangle(City *city, Rect target, Coord from, MemoryArena *memoryArena)
{
	Coord result = from;

	int32 distance;
	Building *fromBuilding = getBuildingAtPosition(city, from);
	if (fromBuilding)
	{
		distance = manhattanDistance(target, fromBuilding->footprint);
	}
	else
	{
		distance = manhattanDistance(target, from);
	}

	if (distance == 0)
	{
		// Already there
		result = from;
	}
	else if (distance == 1)
	{
		// Just walk right in
		result = iCentre(target);
		// if (from.x < target.x) result.x = from.x + 1;
		// else if (from.x >= (target.x + target.w)) result.x = from.x - 1;
		// else if (from.y < target.y) result.y = from.y + 1;
		// else if (from.y >= (target.y + target.h)) result.y = from.y - 1;
	}
	else
	{
		// A-star!
		TemporaryMemoryArena tempArena = beginTemporaryMemory(memoryArena);
		PathingNode *nodes = PushArray(&tempArena, PathingNode, city->width * city->height);
		PathingNode *startNode = nodes + tileIndex(city, from.x, from.y);
		*startNode = {true, from, 0, distance, 0, 0};
		PathingNode *openQueue = null;

		if (fromBuilding
		&& !buildingDefinitions[fromBuilding->archetype].isPath)
		{
			// Add all adjacent path nodes to the openQueue

			for (int x=fromBuilding->footprint.x; x<fromBuilding->footprint.x + fromBuilding->footprint.w; x++)
			{
				_addPathNodeToQueue(city, nodes, coord(x, fromBuilding->footprint.y-1), startNode, &openQueue, target);
				_addPathNodeToQueue(city, nodes, coord(x, fromBuilding->footprint.y + fromBuilding->footprint.h), startNode, &openQueue, target);
			}

			for (int y=fromBuilding->footprint.y; y<fromBuilding->footprint.y + fromBuilding->footprint.h; y++)
			{
				_addPathNodeToQueue(city, nodes, coord(fromBuilding->footprint.x-1, y), startNode, &openQueue, target);
				_addPathNodeToQueue(city, nodes, coord(fromBuilding->footprint.x + fromBuilding->footprint.w, y), startNode, &openQueue, target);
			}
		}
		else
		{
			openQueue = startNode;
		}

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
					_addPathNodeToQueue(city, nodes, adjacents[adjacentIndex], current, &openQueue, target);
				}
			}
		}

		endTemporaryMemory(&tempArena);
	}

	return result;
}