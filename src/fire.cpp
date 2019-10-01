#pragma once

void initFireLayer(FireLayer *layer, City *city, MemoryArena *gameArena)
{
	s32 cityArea = areaOf(city->bounds);

	layer->maxFireRadius = 4;
	layer->tileFireProximityEffect = allocateMultiple<u16>(gameArena, cityArea);
	fillMemory<u16>(layer->tileFireProximityEffect, 0, cityArea);

	layer->tileTotalFireRisk = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileTotalFireRisk, 0, cityArea);
	layer->tileFireProtection = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileFireProtection, 0, cityArea);
	layer->tileOverallFireRisk = allocateMultiple<u8>(gameArena, cityArea);
	fillMemory<u8>(layer->tileOverallFireRisk, 0, cityArea);

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

	// Assets
	// TODO: @AssetPacks
	addTiledSprites("fire"_s, "fire.png"_s, 1, 1, 1, 1, false);
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
			!rectIt.isDone;
			next(&rectIt))
		{
			Rect2I dirtyRect = getValue(rectIt);
			setRegion<u16>(layer->tileFireProximityEffect, city->bounds.w, city->bounds.h, dirtyRect, 0);

			Rect2I expandedRect = expand(dirtyRect, layer->maxFireRadius);
			Rect2I affectedSectors = getSectorsCovered(&layer->sectors, expandedRect);

			for (s32 sy = affectedSectors.y; sy < affectedSectors.y + affectedSectors.h; sy++)
			{
				for (s32 sx = affectedSectors.x; sx < affectedSectors.x + affectedSectors.w; sx++)
				{
					FireSector *sector = getSector(&layer->sectors, sx, sy);

					for (auto it = iterate(&sector->activeFires); hasNext(&it); next(&it))
					{
						Fire *fire = get(it);
						if (contains(expandedRect, fire->pos))
						{
							// TODO: Different "strengths" of fire should have different effects
							EffectRadius fireEffect = {};
							fireEffect.centreValue = 255;
							fireEffect.outerValue = 20;
							fireEffect.radius = layer->maxFireRadius;

							applyEffect<u16>(city, &fireEffect, v2(fire->pos), Effect_Add, layer->tileFireProximityEffect, dirtyRect);
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
				setRegion<u8>(layer->tileFireProtection, city->bounds.w, city->bounds.h, sector->bounds, 0);
				for (auto it = iterate(&layer->fireProtectionBuildings); hasNext(&it); next(&it))
				{
					Building *building = getBuilding(city, getValue(it));
					if (building != null)
					{
						BuildingDef *def = getBuildingDef(building);

						// TODO: Building effectiveness based on budget
						f32 effectiveness = 1.0f;

						if (!buildingHasPower(building))
						{
							effectiveness = 0.4f; // @Balance
						}

						applyEffect(city, &def->fireProtection, centreOf(building->footprint), Effect_Max, layer->tileFireProtection, sector->bounds, effectiveness);
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
					u16 fireProximityEffect = getTileValue(city, layer->tileFireProximityEffect, x, y);
					if (fireProximityEffect > 0)
					{
						tileFireRisk += (tileFireRisk * 4.0f * clamp01(fireProximityEffect / 255.0f));
					}

					u8 totalRisk = clamp01AndMap_u8(tileFireRisk);
					setTile(city, layer->tileTotalFireRisk, x, y, totalRisk);

					// TODO: Balance this! It feels over-powered, even at 50%.
					// Possibly fire stations shouldn't affect risk from active fires?
					f32 protectionPercent = getTileValue(city, layer->tileFireProtection, x, y) * 0.01f;
					u8 result = lerp<u8>(totalRisk, 0, protectionPercent * 0.5f);
					setTile(city, layer->tileOverallFireRisk, x, y, result);
				}
			}
		}
	}
}

void drawFireDataLayer(City *city, Rect2I visibleTileBounds)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	FireLayer *layer = &city->fireLayer;

	// @Copypasta drawLandValueDataLayer() - the same TODO's apply!

#if 1
	u8 *data = copyRegion(layer->tileOverallFireRisk, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);

	Array<V4> *palette = getPalette("risk"_s);

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, (u16)palette->count, palette->items);
#else
	// Just draw the protection
	u8 *data = copyRegion(layer->tileFireProtection, city->bounds.w, city->bounds.h, visibleTileBounds, tempArena);

	Array<V4> *palette = getPalette("service_coverage"s);

	drawGrid(&renderer->worldOverlayBuffer, rect2(visibleTileBounds), renderer->shaderIds.untextured, visibleTileBounds.w, visibleTileBounds.h, data, (u16)palette->count, palette->items);
#endif

	// Highlight fire stations
	drawBuildingHighlights(city, &layer->fireProtectionBuildings);
	drawBuildingEffectRadii(city, &layer->fireProtectionBuildings, &BuildingDef::fireProtection);
}

void drawFires(City *city, Rect2I visibleTileBounds)
{
	FireLayer *layer = &city->fireLayer;

	// TODO: Particle effects for fire instead of this rubbish sprite thing

	if (layer->activeFireCount > 0)
	{
		SpriteGroup *fireSprites = getSpriteGroup("fire"_s);
		Sprite *sprite = getSprite(fireSprites, 0);
		V4 colorWhite = makeWhite();

		DrawRectsGroup *group = beginRectsGroupTextured(&renderer->worldBuffer, sprite->texture, renderer->shaderIds.pixelArt, layer->activeFireCount);

		Rect2I affectedSectors = getSectorsCovered(&layer->sectors, visibleTileBounds);
		for (s32 sy = affectedSectors.y; sy < affectedSectors.y + affectedSectors.h; sy++)
		{
			for (s32 sx = affectedSectors.x; sx < affectedSectors.x + affectedSectors.w; sx++)
			{
				FireSector *sector = getSector(&layer->sectors, sx, sy);

				for (auto it = iterate(&sector->activeFires); hasNext(&it); next(&it))
				{
					Fire *fire = get(it);

					if (contains(visibleTileBounds, fire->pos))
					{
						addSpriteRect(group, sprite, rectXYWHi(fire->pos.x, fire->pos.y, 1, 1), colorWhite);
					}
				}
			}
		}

		endRectsGroup(group);
	}
}

Fire *findFireAt(City *city, s32 x, s32 y)
{
	FireLayer *layer = &city->fireLayer;

	Fire *result = null;

	if (layer->activeFireCount > 0)
	{
		FireSector *sector = getSectorAtTilePos(&layer->sectors, x, y);
		Indexed<Fire *> existingFire = findFirst(&sector->activeFires, [=](Fire *fire) { return fire->pos.x == x && fire->pos.y == y; });

		result = existingFire.value;
	}

	return result;
}

void startFireAt(City *city, s32 x, s32 y)
{
	FireLayer *layer = &city->fireLayer;

	Fire *existingFire = findFireAt(city, x, y);
	if (existingFire != null)
	{
		// Fire already exists!
		// TODO: make the fire stronger?
	}
	else
	{
		FireSector *sector = getSectorAtTilePos(&layer->sectors, x, y);

		Fire *fire = appendBlank(&sector->activeFires);
		fire->pos.x = x;
		fire->pos.y = y;

		layer->activeFireCount++;

		markRectAsDirty(&layer->dirtyRects, irectXYWH(x, y, 1, 1));

		// Tell the building it's on fire
		Building *building = getBuildingAt(city, x, y);
		if (building != null)
		{
			addProblem(building, BuildingProblem_Fire);
		}
	}
}

void removeFireAt(City *city, s32 x, s32 y)
{
	FireLayer *layer = &city->fireLayer;

	FireSector *sectorAtPosition = getSectorAtTilePos(&layer->sectors, x, y);
	s32 removedFires = removeAll(&sectorAtPosition->activeFires, [=](Fire fire) { return fire.pos.x == x && fire.pos.y == y; }, 1);
	if (removedFires > 0)
	{
		markRectAsDirty(&layer->dirtyRects, irectXYWH(x, y, 1, 1));

		layer->activeFireCount -= removedFires;

		// Figure out if the building at the position still has some fire
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
						Fire *fire = get(it);

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
	return getTileValue(city, city->fireLayer.tileTotalFireRisk, x, y);
}

f32 getFireProtectionPercentAt(City *city, s32 x, s32 y)
{
	return getTileValue(city, city->fireLayer.tileFireProtection, x, y) * 0.01f;
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
		formatInt(getTileValue(city, layer->tileFireProximityEffect, x, y)),
	}));

	window_label(context, myprintf("Fire protection: {0}%"_s, {
		formatFloat(getFireProtectionPercentAt(city, x, y) * 100.0f, 0)
	}));

	window_label(context, myprintf("Resulting chance of fire: {0}%"_s, {
		formatFloat(getTileValue(city, layer->tileOverallFireRisk, x, y) / 2.55f, 1)
	}));
}
