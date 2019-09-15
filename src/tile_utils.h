#pragma once

struct EffectRadius
{
	s32 centreValue;
	s32 radius;
	s32 outerValue;
};
inline bool hasEffect(EffectRadius *effectRadius) { return effectRadius->radius > 0; }

enum EffectType
{
	Effect_Add,
	Effect_Max,
};
template<typename T>
void applyEffect(City *city, EffectRadius *effectRadius, V2I effectCentre, EffectType type, T *tileValues, Rect2I region);

// NB: This is a REALLY slow function! It's great for throwing in as a temporary solution, but
// if you want to use it a lot, then a per-tile distance array that's modified with updateDistances()
// when the map changes is much faster.
template<typename Filter>
s32 calculateDistanceTo(City *city, s32 x, s32 y, s32 maxDistanceToCheck, Filter filter);

void updateDistances(City *city, u8 *tileDistance, Rect2I dirtyRect, u8 maxDistance);
void updateDistances(City *city, u8 *tileDistance, DirtyRects *dirtyRects, u8 maxDistance);