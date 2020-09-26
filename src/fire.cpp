#pragma once

void initFireLayer(FireLayer *layer, City *city, MemoryArena *gameArena)
{
	layer->fundingLevel = 1.0f;

	layer->maxFireRadius = 4;
	layer->tileFireProximityEffect = allocateArray2<u16>(gameArena, city->bounds.w, city->bounds.h);
	fill<u16>(&layer->tileFireProximityEffect, 0);

	layer->tileTotalFireRisk = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
	fill<u8>(&layer->tileTotalFireRisk, 0);
	layer->tileFireProtection = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
	fill<u8>(&layer->tileFireProtection, 0);
	layer->tileOverallFireRisk = allocateArray2<u8>(gameArena, city->bounds.w, city->bounds.h);
	fill<u8>(&layer->tileOverallFireRisk, 0);

	initChunkedArray(&layer->fireProtectionBuildings, &city->buildingRefsChunkPool);

	initDirtyRects(&layer->dirtyRects, gameArena, layer->maxFireRadius, city->bounds);

	layer->activeFireCount = 0;
	initChunkPool(&layer->firePool, gameArena, 64);
	initSectorGrid(&layer->sectors, gameArena, city->bounds.w, city->bounds.h, 16, 8);
	for (s32 sectorIndex = 0; sectorIndex < getSectorCount(&layer->sectors); sectorIndex++)
	{
		FireSector *sector = &layer->sectors.sectors[sectorIndex];

		initChunkedArray(&sector->activeFires, &layer->firePool);
	}
}

inline void markFireLayerDirty(FireLayer *layer, Rect2I bounds)
{
	markRectAsDirty(&layer->dirtyRects, bounds);
}

void updateFireLayer(City *city, FireLayer *layer)
{
	DEBUG_FUNCTION_T(DCDT_Simulation);

	if (isDirty(&layer->dirtyRects))
	{
		DEBUG_BLOCK_T("updateFireLayer: building effects", DCDT_Simulation);

		// Recalculate fire distances
		for (auto rectIt = iterate(&layer->dirtyRects.rects);
			hasNext(&rectIt);
			next(&rectIt))
		{
			Rect2I dirtyRect = getValue(&rectIt);
			fillRegion<u16>(&layer->tileFireProximityEffect, dirtyRect, 0);

			Rect2I expandedRect = expand(dirtyRect, layer->maxFireRadius);
			Rect2I affectedSectors = getSectorsCovered(&layer->sectors, expandedRect);

			for (s32 sy = affectedSectors.y; sy < affectedSectors.y + affectedSectors.h; sy++)
			{
				for (s32 sx = affectedSectors.x; sx < affectedSectors.x + affectedSectors.w; sx++)
				{
					FireSector *sector = getSector(&layer->sectors, sx, sy);

					for (auto it = iterate(&sector->activeFires); hasNext(&it); next(&it))
					{
						Fire *fire = get(&it);
						if (contains(expandedRect, fire->pos))
						{
							// TODO: Different "strengths" of fire should have different effects
							EffectRadius fireEffect = {};
							fireEffect.centreValue = 255;
							fireEffect.outerValue = 20;
							fireEffect.radius = layer->maxFireRadius;

							applyEffect(&fireEffect, v2(fire->pos), Effect_Add, &layer->tileFireProximityEffect, dirtyRect);
						}
					}
				}
			}
		}

		clearDirtyRects(&layer->dirtyRects);
	}

	{
		DEBUG_BLOCK_T("updateFireLayer: overall calculation", DCDT_Simulation);

		for (s32 i = 0; i < layer->sectors.sectorsToUpdatePerTick; i++)
		{
			FireSector *sector = getNextSector(&layer->sectors);

			{
				DEBUG_BLOCK_T("updateFireLayer: building fire protection", DCDT_Simulation);
				// Building fire protection
				fillRegion<u8>(&layer->tileFireProtection, sector->bounds, 0);
				for (auto it = iterate(&layer->fireProtectionBuildings); hasNext(&it); next(&it))
				{
					Building *building = getBuilding(city, getValue(&it));
					if (building != null)
					{
						BuildingDef *def = getBuildingDef(building);

						f32 effectiveness = layer->fundingLevel;

						if (!buildingHasPower(building))
						{
							effectiveness *= 0.4f; // @Balance
						}

						applyEffect(&def->fireProtection, centreOf(building->footprint), Effect_Max, &layer->tileFireProtection, sector->bounds, effectiveness);
					}
				}
			}

			for (s32 y = sector->bounds.y; y < sector->bounds.y + sector->bounds.h; y++)
			{
				for (s32 x = sector->bounds.x; x < sector->bounds.x + sector->bounds.w; x++)
				{
					f32 tileFireRisk = 0.0f;

					Building *building = getBuildingAt(city, x, y);
					if (building)
					{
						BuildingDef *def = getBuildingDef(building);
						tileFireRisk += def->fireRisk;
					}

					// Being near a fire has a HUGE risk!
					// TODO: Balance this! Currently it's a multiplier on the base building risk,
					// because if we just ADD a value, then we get fire risk on empty tiles.
					// But, the balance is definitely off.
					u16 fireProximityEffect = layer->tileFireProximityEffect.get(x, y);
					if (fireProximityEffect > 0)
					{
						tileFireRisk += (tileFireRisk * 4.0f * clamp01(fireProximityEffect / 255.0f));
					}

					u8 totalRisk = clamp01AndMap_u8(tileFireRisk);
					layer->tileTotalFireRisk.set(x, y, totalRisk);

					// TODO: Balance this! It feels over-powered, even at 50%.
					// Possibly fire stations shouldn't affect risk from active fires?
					f32 protectionPercent = layer->tileFireProtection.get(x, y) * 0.01f;
					u8 result = lerp<u8>(totalRisk, 0, protectionPercent * 0.5f);
					layer->tileOverallFireRisk.set(x, y, result);
				}
			}
		}
	}
}

Indexed<Fire*> findFireAt(City *city, s32 x, s32 y)
{
	FireLayer *layer = &city->fireLayer;

	Indexed<Fire*> result = makeNullIndexedValue<Fire*>();

	if (layer->activeFireCount > 0)
	{
		FireSector *sector = getSectorAtTilePos(&layer->sectors, x, y);
		result = findFirst(&sector->activeFires, [=](Fire *fire) { return fire->pos.x == x && fire->pos.y == y; });
	}

	return result;
}

void startFireAt(City *city, s32 x, s32 y)
{
	Indexed<Fire*> existingFire = findFireAt(city, x, y);
	if (existingFire.value != null)
	{
		// Fire already exists!
		// TODO: make the fire stronger?
		logWarn("Fire at {0},{1} already exists!"_s, {formatInt(x), formatInt(y)});
	}
	else
	{
		addFireRaw(city, x, y);
	}
}

void addFireRaw(City *city, s32 x, s32 y)
{
	// NB: We don't care if the fire already exists. startFireAt() already checks for this case.

	FireLayer *layer = &city->fireLayer;

	FireSector *sector = getSectorAtTilePos(&layer->sectors, x, y);

	Fire *fire = appendBlank(&sector->activeFires);

	fire->pos = v2i(x, y);
	fire->entity = addEntity(city, EntityType_Fire, fire);
	// TODO: Probably most of this wants to be moved into addEntity()
	fire->entity->bounds = rectXYWHi(x, y, 1, 1);
	fire->entity->sprite = getSprite(getSpriteGroup("e_fire_1x1"_s), randomNext(&globalAppState.cosmeticRandom));

	layer->activeFireCount++;

	markRectAsDirty(&layer->dirtyRects, irectXYWH(x, y, 1, 1));

	// Tell the building it's on fire
	// TODO: Remove this! @BuildingProblem
	Building *building = getBuildingAt(city, x, y);
	if (building != null)
	{
		addProblem(building, BuildingProblem_Fire);
	}
}

void updateFire(City * /*city*/, Fire * /*fire*/)
{
	// TODO: Burn baby burn, disco inferno
}

void removeFireAt(City *city, s32 x, s32 y)
{
	FireLayer *layer = &city->fireLayer;

	FireSector *sectorAtPosition = getSectorAtTilePos(&layer->sectors, x, y);
	Indexed<Fire*> fireAtPosition = findFirst(&sectorAtPosition->activeFires, [=](Fire *fire) { return fire->pos.x == x && fire->pos.y == y; });

	if (fireAtPosition.value != null)
	{
		// Remove it!
		removeEntity(city, fireAtPosition.value->entity);
		removeIndex(&sectorAtPosition->activeFires, fireAtPosition.index);
		layer->activeFireCount--;

		markRectAsDirty(&layer->dirtyRects, irectXYWH(x, y, 1, 1));

		// Figure out if the building at the position still has some fire
		// TODO: Move problem detection, Building should be responsible for it @BuildingProblem
		Building *building = getBuildingAt(city, x, y);
		if (building != null)
		{
			bool foundFire = false;

			Rect2I footprintSectors = getSectorsCovered(&layer->sectors, building->footprint);
			for (s32 sy = footprintSectors.y;
				sy < footprintSectors.y + footprintSectors.h && !foundFire;
				sy++)
			{
				for (s32 sx = footprintSectors.x;
					sx < footprintSectors.x + footprintSectors.w && !foundFire;
					sx++)
				{
					FireSector *sector = getSector(&layer->sectors, sx, sy);
					for (auto it = iterate(&sector->activeFires); hasNext(&it); next(&it))
					{
						Fire *fire = get(&it);

						if (contains(building->footprint, fire->pos))
						{
							foundFire = true;
							break;
						}
					}
				}
			}
			
			if (!foundFire)
			{
				removeProblem(building, BuildingProblem_Fire);
			}
		}
	}
}

void registerFireProtectionBuilding(FireLayer *layer, Building *building)
{
	append(&layer->fireProtectionBuildings, getReferenceTo(building));
}

void unregisterFireProtectionBuilding(FireLayer *layer, Building *building)
{
	bool success = findAndRemove(&layer->fireProtectionBuildings, getReferenceTo(building));
	ASSERT(success);
}

u8 getFireRiskAt(City *city, s32 x, s32 y)
{
	return city->fireLayer.tileTotalFireRisk.get(x, y);
}

f32 getFireProtectionPercentAt(City *city, s32 x, s32 y)
{
	return city->fireLayer.tileFireProtection.get(x, y) * 0.01f;
}

void debugInspectFire(WindowContext *context, City *city, s32 x, s32 y)
{
	FireLayer *layer = &city->fireLayer;

	window_label(context, "*** FIRE INFO ***"_s);

	window_label(context, myprintf("There are {0} fire protection buildings and {1} active fires in the city."_s, {
		formatInt(layer->fireProtectionBuildings.count),
		formatInt(layer->activeFireCount)
	}));

	Building *buildingAtPos = getBuildingAt(city, x, y);
	f32 buildingFireRisk = 100.0f * ((buildingAtPos == null) ? 0.0f : getBuildingDef(buildingAtPos)->fireRisk);

	window_label(context, myprintf("Fire risk: {0}, from:\n- Building: {1}%\n- Nearby fires: {2}"_s, {
		formatInt(getFireRiskAt(city, x, y)),
		formatFloat(buildingFireRisk, 1),
		formatInt(layer->tileFireProximityEffect.get(x, y)),
	}));

	window_label(context, myprintf("Fire protection: {0}%"_s, {
		formatFloat(getFireProtectionPercentAt(city, x, y) * 100.0f, 0)
	}));

	window_label(context, myprintf("Resulting chance of fire: {0}%"_s, {
		formatFloat(layer->tileOverallFireRisk.get(x, y) / 2.55f, 1)
	}));
}
