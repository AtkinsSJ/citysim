#pragma once

inline s32 powerGroupAt(City *city, s32 x, s32 y)
{
	s32 result = 0;

	if (tileExists(city, x, y))
	{
		result = city->powerLayer.data[tileIndex(city, x, y)];
	}

	return result;
}

void initialisePowerLayer(MemoryArena *gameArena, PowerLayer *layer, s32 tileCount)
{
	layer->data = PushArray(gameArena, s32, tileCount);
	initialiseArray(&layer->groups, 64);
}

bool floodFillPowerConnectivity(City *city, s32 x, s32 y, s32 fillValue)
{
	bool didCreateNewGroup = false;

	city->powerLayer.data[tileIndex(city, x, y)] = fillValue;

	if (powerGroupAt(city, x-1, y) == -1)
	{
		floodFillPowerConnectivity(city, x-1, y, fillValue);
		didCreateNewGroup = true;
	}
	
	if (powerGroupAt(city, x+1, y) == -1)
	{
		floodFillPowerConnectivity(city, x+1, y, fillValue);
		didCreateNewGroup = true;
	}

	if (powerGroupAt(city, x, y+1) == -1)
	{
		floodFillPowerConnectivity(city, x, y+1, fillValue);
		didCreateNewGroup = true;
	}

	if (powerGroupAt(city, x, y-1) == -1)
	{
		floodFillPowerConnectivity(city, x, y-1, fillValue);
		didCreateNewGroup = true;
	}

	return didCreateNewGroup;
}

void recalculatePowerConnectivity(City *city)
{
	// This is a flood fill.
	// First, normalise things so path tiles are -1, others are 0
	// Then, iterate over the tiles and flood fill from each -1 value.

	// Reset things to 0/-1
	s32 maxTileIndex = city->width * city->height;
	for (s32 tileIndex = 0; tileIndex < maxTileIndex; ++tileIndex)
	{
		if (city->powerLayer.data[tileIndex] != 0)
		{
			city->powerLayer.data[tileIndex] = -1;
		}
	}

	clear(&city->powerLayer.groups);

	// Find -1 tiles
	for (s32 y=0, tileIndex = 0; y<city->height; y++)
	{
		for (s32 x=0; x<city->width; x++, tileIndex++)
		{
			if (city->powerLayer.data[tileIndex] == -1)
			{
				// Flood fill from here!
				if (floodFillPowerConnectivity(city, x, y, ++city->powerLayer.groups.count + 1))
				{
					PowerGroup *newGroup = appendBlank(&city->powerLayer.groups);
					newGroup->production = 0;
					newGroup->consumption = 0;
				}
			}
		}
	}

	// Find all power production/consumption buildings and add them up
	// TODO: some kind of building-type query would probably be better?
}