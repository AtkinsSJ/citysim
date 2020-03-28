#pragma once

template<typename T>
void applyEffect(EffectRadius *effectRadius, V2 effectCentre, EffectType type, Array2<T> *tileValues, Rect2I region, f32 scale)
{
	DEBUG_FUNCTION();
	
	f32 radius = (f32) effectRadius->radius;
	f32 invRadius = 1.0f / radius;
	f32 radius2 = (f32)(radius * radius);

	f32 centreValue = effectRadius->centreValue * scale;
	f32 outerValue  = effectRadius->outerValue  * scale;

	Rect2I possibleEffectArea = irectXYWH(floor_s32(effectCentre.x - radius), floor_s32(effectCentre.y - radius), ceil_s32(radius + radius), ceil_s32(radius + radius));
	possibleEffectArea = intersect(possibleEffectArea, region);
	for (s32 y = possibleEffectArea.y; y < possibleEffectArea.y + possibleEffectArea.h; y++)
	{
		for (s32 x = possibleEffectArea.x; x < possibleEffectArea.x + possibleEffectArea.w; x++)
		{
			f32 distance2FromSource = lengthSquaredOf(x - effectCentre.x, y - effectCentre.y);
			if (distance2FromSource <= radius2)
			{
				f32 contributionF = lerp(centreValue, outerValue, sqrt_f32(distance2FromSource) * invRadius);
				T contribution = (T)floor_s32(contributionF);

				if (contribution == 0) continue;

				switch (type)
				{
					case Effect_Add: {
						T originalValue = tileValues->get(x, y);

						// This clamp is probably unnecessary but just in case.
						T newValue = clamp<T>(originalValue + contribution, minPossibleValue<T>(), maxPossibleValue<T>());
						tileValues->set(x, y, newValue);
					} break;

					case Effect_Max: {
						T originalValue = tileValues->get(x, y);
						T newValue = max(originalValue, contribution);
						tileValues->set(x, y, newValue);
					} break;

					INVALID_DEFAULT_CASE;
				}
			}
		}
	}
}

// The simplest possible algorithm is, just spread the 0s out that we marked above.
// (If a tile is not 0, set it to the min() of its 8 neighbours, plus 1.)
// We have to iterate through the area `maxDistance` times, but it should be fast enough probably.
void updateDistances(Array2<u8> *tileDistance, Rect2I dirtyRect, u8 maxDistance)
{
	DEBUG_FUNCTION();
	
	for (s32 iteration = 0; iteration < maxDistance; iteration++)
	{
		for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
		{
			for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
			{
				if (tileDistance->get(x, y) != 0)
				{
					u8 minDistance = min(
						tileDistance->getIfExists(x-1, y-1, 255),
						tileDistance->getIfExists(x  , y-1, 255),
						tileDistance->getIfExists(x+1, y-1, 255),
						tileDistance->getIfExists(x-1, y  , 255),
					//	tileDistance->getIfExists(x  , y  , 255),
						tileDistance->getIfExists(x+1, y  , 255),
						tileDistance->getIfExists(x-1, y+1, 255),
						tileDistance->getIfExists(x  , y+1, 255),
						tileDistance->getIfExists(x+1, y+1, 255)
					);

					if (minDistance != 255)  minDistance++;
					if (minDistance > maxDistance)  minDistance = 255;

					tileDistance->set(x, y, minDistance);
				}
			}
		}
	}
}

// @CopyPasta the other updateDistances().
void updateDistances(Array2<u8> *tileDistance, DirtyRects *dirtyRects, u8 maxDistance)
{
	DEBUG_FUNCTION();
	
	for (s32 iteration = 0; iteration < maxDistance; iteration++)
	{
		for (auto it = iterate(&dirtyRects->rects);
			hasNext(&it);
			next(&it))
		{
			Rect2I dirtyRect = getValue(&it);

			for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
			{
				for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
				{
					if (tileDistance->get(x, y) != 0)
					{
						u8 minDistance = min(
							tileDistance->getIfExists(x-1, y-1, 255),
							tileDistance->getIfExists(x  , y-1, 255),
							tileDistance->getIfExists(x+1, y-1, 255),
							tileDistance->getIfExists(x-1, y  , 255),
						//	tileDistance->getIfExists(x  , y  , 255),
							tileDistance->getIfExists(x+1, y  , 255),
							tileDistance->getIfExists(x-1, y+1, 255),
							tileDistance->getIfExists(x  , y+1, 255),
							tileDistance->getIfExists(x+1, y+1, 255)
						);

						if (minDistance != 255)  minDistance++;
						if (minDistance > maxDistance)  minDistance = 255;

						tileDistance->set(x, y, minDistance);
					}
				}
			}
		}
	}
}

template<typename Filter>
s32 calculateDistanceTo(City *city, s32 x, s32 y, s32 maxDistanceToCheck, Filter filter)
{
	DEBUG_FUNCTION();
	
	s32 result = s32Max;

	if (filter(city, x, y))
	{
		result = 0;
	}
	else
	{
		bool done = false;

		for (s32 distance = 1;
			 !done && distance <= maxDistanceToCheck;
			 distance++)
		{
			s32 leftX   = x - distance;
			s32 rightX  = x + distance;
			s32 bottomY = y - distance;
			s32 topY    = y + distance;

			for (s32 px = leftX; px <= rightX; px++)
			{
				if (filter(city, px, bottomY))
				{
					result = distance;
					done = true;
					break;
				}

				if (filter(city, px, topY))
				{
					result = distance;
					done = true;
					break;
				}
			}

			if (done) break;

			for (s32 py = bottomY; py <= topY; py++)
			{
				if (filter(city, leftX, py))
				{
					result = distance;
					done = true;
					break;
				}

				if (filter(city, rightX, py))
				{
					result = distance;
					done = true;
					break;
				}
			}
		}
	}

	return result;
}

template<typename Iterable>
void drawBuildingHighlights(City *city, Iterable *buildingRefs)
{
	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	if (buildingRefs->count > 0)
	{
		Array<V4> *buildingsPalette = getPalette("service_buildings"_s);
		s32 paletteIndexPowered   = 0;
		s32 paletteIndexUnpowered = 1;

		DrawRectsGroup *buildingHighlights = beginRectsGroupUntextured(&renderer->worldOverlayBuffer, renderer->shaderIds.untextured, buildingRefs->count);
		for (auto it = iterate(buildingRefs); hasNext(&it); next(&it))
		{
			Building *building = getBuilding(city, getValue(&it));
			// NB: If we're doing this in a separate loop, we could crop out buildings that aren't in the visible tile bounds
			if (building != null)
			{
				s32 paletteIndex = (buildingHasPower(building) ? paletteIndexPowered : paletteIndexUnpowered);
				addUntexturedRect(buildingHighlights, rect2(building->footprint), (*buildingsPalette)[paletteIndex]);
			}
		}
		endRectsGroup(buildingHighlights);
	}
}

template<typename Iterable>
void drawBuildingEffectRadii(City *city, Iterable *buildingRefs, EffectRadius BuildingDef::* effectMember)
{
	//
	// Leaving a note here because it's the first time I've used a pointer-to-member, and it's
	// weird and confusing and the syntax is odd!
	//
	// You declare the variable/parameter like this:
	//    EffectRadius BuildingDef::* effectMember;
	// Essentially it's just a regular variable definition, with the struct name in the middle and ::*
	//
	// Then, we use it like this:
	//    EffectRadius *effect = &(def->*effectMember);
	// (The important part is      ^^^^^^^^^^^^^^^^^^)
	// So, you access the member like you would normally with a . or ->, except you put a * before
	// the member name to show that it's a member pointer instead of an actual member.
	//
	// Now that I've written it out, it's not so bad, but it was really hard to look up about because
	// I assumed this would be a template-related feature, and so all the examples I found were super
	// template heavy and abstract and confusing. But no! It's a built-in feature that's actually not
	// too complicated. I might use this more now that I know about it.
	//
	// - Sam, 18/09/2019
	//

	DEBUG_FUNCTION_T(DCDT_GameUpdate);

	if (buildingRefs->count > 0)
	{
		Array<V4> *ringsPalette = getPalette("coverage_radius"_s);
		s32 paletteIndexPowered   = 0;
		s32 paletteIndexUnpowered = 1;

		DrawRingsGroup *buildingRadii = beginRingsGroup(&renderer->worldOverlayBuffer, buildingRefs->count);

		for (auto it = iterate(buildingRefs); hasNext(&it); next(&it))
		{
			Building *building = getBuilding(city, getValue(&it));
			// NB: We don't filter buildings outside of the visibleTileBounds because their radius might be
			// visible even if the building isn't!
			if (building != null)
			{
				BuildingDef *def = getBuildingDef(building);
				EffectRadius *effect = &(def->*effectMember);
				if (hasEffect(effect))
				{
					s32 paletteIndex = (buildingHasPower(building) ? paletteIndexPowered : paletteIndexUnpowered);
					addRing(buildingRadii, centreOf(building->footprint), (f32) effect->radius, 0.5f, (*ringsPalette)[paletteIndex]);
				}
			}
		}

		endRingsGroup(buildingRadii);
	}
}
