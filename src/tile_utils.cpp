#pragma once

template<typename T>
void applyEffect(City *city, EffectRadius *effectRadius, V2I effectCentre, EffectType type, T *tileValues, Rect2I region)
{
	DEBUG_FUNCTION();
	
	s32 diameter = 1 + (effectRadius->radius * 2);
	f32 invRadius = 1.0f / (f32) effectRadius->radius;
	Rect2I possibleEffectArea = irectCentreSize(effectCentre, v2i(diameter, diameter));
	possibleEffectArea = intersect(possibleEffectArea, region);

	for (s32 y = possibleEffectArea.y; y < possibleEffectArea.y + possibleEffectArea.h; y++)
	{
		for (s32 x = possibleEffectArea.x; x < possibleEffectArea.x + possibleEffectArea.w; x++)
		{
			f32 distanceFromSource = lengthOf(x - effectCentre.x, y - effectCentre.y);
			if (distanceFromSource <= effectRadius->radius)
			{
				f32 contributionF = lerp((f32)effectRadius->centreValue, (f32)effectRadius->outerValue, (distanceFromSource * invRadius));
				T contribution = (T)floor_s32(contributionF);

				if (contribution == 0) continue;

				switch (type)
				{
					case Effect_Add: {
						T originalValue = getTileValue(city, tileValues, x, y);

						// This clamp is probably unnecessary but just in case.
						T newValue = clamp<T>(originalValue + contribution, s16Min, s16Max);
						setTile(city, tileValues, x, y, newValue);
					} break;

					INVALID_DEFAULT_CASE;
				}
			}
		}
	}
}

void updateDistances(City *city, u8 *tileDistance, Rect2I dirtyRect, u8 maxDistance)
{
	DEBUG_FUNCTION();
	ASSERT(contains(city->bounds, dirtyRect));
	
	for (s32 iteration = 0; iteration < maxDistance; iteration++)
	{
		for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
		{
			for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
			{
				if (getTileValue(city, tileDistance, x, y) != 0)
				{
					u8 minDistance = min(
						getTileValue<u8>(city, tileDistance, x-1, y-1),
						getTileValue<u8>(city, tileDistance, x  , y-1),
						getTileValue<u8>(city, tileDistance, x+1, y-1),
						getTileValue<u8>(city, tileDistance, x-1, y  ),
					//	getTileValue<u8>(city, tileDistance, x  , y  ),
						getTileValue<u8>(city, tileDistance, x+1, y  ),
						getTileValue<u8>(city, tileDistance, x-1, y+1),
						getTileValue<u8>(city, tileDistance, x  , y+1),
						getTileValue<u8>(city, tileDistance, x+1, y+1)
					);

					if (minDistance != 255)  minDistance++;
					if (minDistance > maxDistance)  minDistance = 255;

					setTile<u8>(city, tileDistance, x, y, minDistance);
				}
			}
		}
	}
}

// The simplest possible algorithm is, just spread the 0s out that we marked above.
// (If a tile is not 0, set it to the min() of its 8 neighbours, plus 1.)
// We have to iterate through the area `maxDistance` times, but it should be fast enough probably.
void updateDistances(City *city, u8 *tileDistance, DirtyRects *dirtyRects, u8 maxDistance)
{
	DEBUG_FUNCTION();
	
	for (s32 iteration = 0; iteration < maxDistance; iteration++)
	{
		for (auto it = iterate(&dirtyRects->rects);
			!it.isDone;
			next(&it))
		{
			Rect2I dirtyRect = getValue(it);

			for (s32 y = dirtyRect.y; y < dirtyRect.y + dirtyRect.h; y++)
			{
				for (s32 x = dirtyRect.x; x < dirtyRect.x + dirtyRect.w; x++)
				{
					if (getTileValue(city, tileDistance, x, y) != 0)
					{
						u8 minDistance = min(
							getTileValue<u8>(city, tileDistance, x-1, y-1),
							getTileValue<u8>(city, tileDistance, x  , y-1),
							getTileValue<u8>(city, tileDistance, x+1, y-1),
							getTileValue<u8>(city, tileDistance, x-1, y  ),
						//	getTileValue<u8>(city, tileDistance, x  , y  ),
							getTileValue<u8>(city, tileDistance, x+1, y  ),
							getTileValue<u8>(city, tileDistance, x-1, y+1),
							getTileValue<u8>(city, tileDistance, x  , y+1),
							getTileValue<u8>(city, tileDistance, x+1, y+1)
						);

						if (minDistance != 255)  minDistance++;
						if (minDistance > maxDistance)  minDistance = 255;

						setTile<u8>(city, tileDistance, x, y, minDistance);
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
