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
};
template<typename T>
void applyEffect(City *city, EffectRadius *effectRadius, V2I effectCentre, EffectType type, T *tileValues, Rect2I region);

template<typename Filter>
s32 calculateDistanceTo(City *city, s32 x, s32 y, s32 maxDistanceToCheck, Filter filter);

void updateDistances(City *city, u8 *tileDistance, DirtyRects *dirtyRects, u8 maxDistance);
