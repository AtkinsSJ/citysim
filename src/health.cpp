#pragma once

inline f32 getHealthCoveragePercentAt(City *city, s32 x, s32 y)
{
	return city->healthLayer.tileHealthCoverage.get(x, y) * 0.01f;
}

void initHealthLayer(HealthLayer *layer, City *city, MemoryArena *gameArena)
{
	initSectorGrid(&layer->sectors, gameArena, city->bounds.w, city->bounds.h, 16, 8);

	initDirtyRects(&layer->dirtyRects, gameArena, maxLandValueEffectDistance, city->bounds);

	layer->tileHealthCoverage = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
	fill<u8>(&layer->tileHealthCoverage, 0);

	initChunkedArray(&layer->healthBuildings, &city->buildingRefsChunkPool);

	layer->fundingLevel = 1.0f;
}

void updateHealthLayer(City *city, HealthLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	if (isDirty(&layer->dirtyRects))
	{
		DEBUG_BLOCK_T("updateHealthLayer: dirty rects", DCDT_Simulation);

		// TODO: do we actually need dirty rects? I can't think of anything, unless we move the "register building" stuff to that.

		clearDirtyRects(&layer->dirtyRects);
	}

	{
		DEBUG_BLOCK_T("updateHealthLayer: sector updates", DCDT_Simulation);

		for (s32 i = 0; i < layer->sectors.sectorsToUpdatePerTick; i++)
		{
			BasicSector *sector = getNextSector(&layer->sectors);

			{
				DEBUG_BLOCK_T("updateHealthLayer: building health coverage", DCDT_Simulation);
				fillRegion<u8>(&layer->tileHealthCoverage, sector->bounds, 0);
				for (auto it = layer->healthBuildings.iterate(); it.hasNext(); it.next())
				{
					Building *building = getBuilding(city, it.getValue());
					if (building != null)
					{
						BuildingDef *def = getBuildingDef(building);

						// Budget
						f32 effectiveness = layer->fundingLevel;

						// Power
						if (!buildingHasPower(building))
						{
							effectiveness *= 0.4f; // @Balance
						}

						// TODO: Water

						// TODO: Overcrowding

						applyEffect(&def->healthEffect, centreOf(building->footprint), Effect_Max, &layer->tileHealthCoverage, sector->bounds, effectiveness);
					}
				}
			}
		}
	}
}

inline void markHealthLayerDirty(HealthLayer *layer, Rect2I bounds)
{
	markRectAsDirty(&layer->dirtyRects, bounds);
}

void notifyNewBuilding(HealthLayer *layer, BuildingDef *def, Building *building)
{
	if (hasEffect(&def->healthEffect))
	{
		layer->healthBuildings.append(getReferenceTo(building));
	}
}

void notifyBuildingDemolished(HealthLayer *layer, BuildingDef *def, Building *building)
{
	if (hasEffect(&def->healthEffect))
	{
		bool success = layer->healthBuildings.findAndRemove(getReferenceTo(building));
		ASSERT(success);
	}
}
