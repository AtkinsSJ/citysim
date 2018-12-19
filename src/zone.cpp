#include "zone.h"

void initZoneLayer(MemoryArena *memoryArena, ZoneLayer *zoneLayer, s32 tileCount)
{
	zoneLayer->tiles = PushArray(memoryArena, ZoneType, tileCount);
	initChunkedArray(&zoneLayer->emptyRZones,  memoryArena, 256);
	initChunkedArray(&zoneLayer->filledRZones, memoryArena, 256);
	initChunkedArray(&zoneLayer->emptyCZones,  memoryArena, 256);
	initChunkedArray(&zoneLayer->filledCZones, memoryArena, 256);
	initChunkedArray(&zoneLayer->emptyIZones,  memoryArena, 256);
	initChunkedArray(&zoneLayer->filledIZones, memoryArena, 256);
}

bool canZoneTile(City *city, ZoneType zoneType, s32 x, s32 y)
{
	if (!tileExists(city, x, y)) return false;

	s32 tile = tileIndex(city, x, y);

	TerrainDef *tDef = get(&terrainDefs, terrainAt(city, x, y).type);
	if (!tDef->canBuildOn) return false;

	if (city->tileBuildings[tile] != 0) return false;

	// Ignore tiles that are already this zone!
	if (city->zoneLayer.tiles[tile] == zoneType) return false;

	return true;
}

s32 calculateZoneCost(City *city, ZoneType zoneType, Rect2I area)
{
	ZoneDef zoneDef = zoneDefs[zoneType];

	s32 total = 0;

	// For now, you can only zone free tiles, where there are no buildings or obstructions.
	// You CAN zone over other zones though. Just, not if something has already grown in that
	// zone tile.
	// At some point we'll probably want to change this, because my least-favourite thing
	// about SC3K is that clearing a built-up zone means demolishing the buildings then quickly
	// switching to the de-zone tool before something else pops up in the free space!
	// - Sam, 13/12/2018

	for (int y=0; y<area.h; y++)
	{
		for (int x=0; x<area.w; x++)
		{
			if (canZoneTile(city, zoneType, area.x + x, area.y + y))
			{
				total += zoneDef.costPerTile;
			}
		}
	}

	return total;
}

void placeZone(UIState *uiState, City *city, ZoneType zoneType, Rect2I area, bool chargeMoney)
{
	if (chargeMoney)
	{
		s32 cost = calculateZoneCost(city, zoneType, area);
		if (!canAfford(city, cost))
		{
			pushUiMessage(uiState, stringFromChars("Not enough money to zone this."));
			return;
		}
		else
		{
			spend(city, cost);
		}
	}

	for (int y=0; y<area.h; y++) {
		for (int x=0; x<area.w; x++) {
			if (canZoneTile(city, zoneType, area.x + x, area.y + y))
			{
				s32 tile = tileIndex(city, area.x + x, area.y + y);
				city->zoneLayer.tiles[tile] = zoneType;
			}
		}
	}

	// Zones carry power!
	recalculatePowerConnectivity(city);
}

void growZoneBuilding(City *city, BuildingDef *def, Rect2I footprint)
{

}

void growSomeZoneBuildings(City *city)
{
	// Residential
	if (city->residentialDemand > 0)
	{
		s32 remainingDemand = city->residentialDemand;
		s32 minimumDemand = city->residentialDemand / 20; // Stop when we're below 20% of the original demand

		while (remainingDemand > minimumDemand)
		{
			// Find a valid res zone


			// Pick a building def that fits the space and is not more than 10% more than the remaining demand
			// Place it!
		}
	}
}