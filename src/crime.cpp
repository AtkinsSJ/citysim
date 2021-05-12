#pragma once

void initCrimeLayer(CrimeLayer *layer, City *city, MemoryArena *gameArena)
{
	initSectorGrid(&layer->sectors, gameArena, city->bounds.w, city->bounds.h, 16, 8);

	initDirtyRects(&layer->dirtyRects, gameArena, maxLandValueEffectDistance, city->bounds);

	layer->tilePoliceCoverage = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
	fill<u8>(&layer->tilePoliceCoverage, 0);

	layer->totalJailCapacity = 0;
	layer->occupiedJailCapacity = 0;

	layer->fundingLevel = 1.0f;

	initChunkedArray(&layer->policeBuildings, &city->buildingRefsChunkPool);
}

void updateCrimeLayer(City *city, CrimeLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	if (isDirty(&layer->dirtyRects))
	{
		DEBUG_BLOCK_T("updateCrimeLayer: dirty rects", DCDT_Simulation);
		clearDirtyRects(&layer->dirtyRects);
	}

	// Recalculate jail capacity
	// NB: This only makes sense if we assume that building jail capacities can change while
	// the game is running. It'll only happen incredibly rarely during normal play, but during
	// development we have this whole hot-loaded building defs system, so it's better to be safe.
	layer->totalJailCapacity = 0;
	for (auto it = layer->policeBuildings.iterate(); it.hasNext(); it.next())
	{
		Building *building = getBuilding(city, it.getValue());
		if (building != null)
		{
			BuildingDef *def = getBuildingDef(building);
			if (def->jailCapacity > 0)  layer->totalJailCapacity += def->jailCapacity;
		}
	}

	{
		DEBUG_BLOCK_T("updateCrimeLayer: sector updates", DCDT_Simulation);

		for (s32 i = 0; i < layer->sectors.sectorsToUpdatePerTick; i++)
		{
			BasicSector *sector = getNextSector(&layer->sectors);

			{
				DEBUG_BLOCK_T("updateCrimeLayer: building police coverage", DCDT_Simulation);
				fillRegion<u8>(&layer->tilePoliceCoverage, sector->bounds, 0);
				for (auto it = layer->policeBuildings.iterate(); it.hasNext(); it.next())
				{
					Building *building = getBuilding(city, it.getValue());
					if (building != null)
					{
						BuildingDef *def = getBuildingDef(building);

						if (hasEffect(&def->policeEffect))
						{
							// Budget
							f32 effectiveness = layer->fundingLevel;

							if (!buildingHasPower(building))
							{
								effectiveness *= 0.4f; // @Balance

								// TODO: Consider water access too
							}

							applyEffect(&def->policeEffect, centreOf(building->footprint), Effect_Add, &layer->tilePoliceCoverage, sector->bounds, effectiveness);
						}
					}
				}
			}
		}
	}
}

void notifyNewBuilding(CrimeLayer *layer, BuildingDef *def, Building *building)
{
	if (hasEffect(&def->policeEffect) || (def->jailCapacity > 0))
	{
		layer->policeBuildings.append(getReferenceTo(building));
	}
}

void notifyBuildingDemolished(CrimeLayer *layer, BuildingDef *def, Building *building)
{
	if (hasEffect(&def->policeEffect) || (def->jailCapacity > 0))
	{
		bool success = layer->policeBuildings.findAndRemove(getReferenceTo(building));
		ASSERT(success);
	}
}

f32 getPoliceCoveragePercentAt(City *city, s32 x, s32 y)
{
	return city->crimeLayer.tilePoliceCoverage.get(x, y) / 255.0f;
}

void saveCrimeLayer(CrimeLayer *layer, struct BinaryFileWriter *writer)
{
	writer->startSection<SAVSection_Crime>(SAV_CRIME_ID, SAV_CRIME_VERSION);
	SAVSection_Crime crimeSection = {};

	crimeSection.totalJailCapacity = layer->totalJailCapacity;
	crimeSection.occupiedJailCapacity = layer->occupiedJailCapacity;

	writer->endSection<SAVSection_Crime>(&crimeSection);
}

bool loadCrimeLayer(CrimeLayer *layer, City * /*city*/, struct BinaryFileReader *reader)
{
	bool succeeded = false;
	while (reader->startSection(SAV_CRIME_ID, SAV_CRIME_VERSION))
	{
		// Load Crime
		SAVSection_Crime *section = reader->readStruct<SAVSection_Crime>(0);
		if (!section) break;

		layer->totalJailCapacity    = section->totalJailCapacity;
		layer->occupiedJailCapacity = section->occupiedJailCapacity;

		succeeded = true;
		break;
	}

	return succeeded;
}
