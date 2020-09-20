#pragma once

enum EntityType
{
	EntityType_None,
	EntityType_Building,
	EntityType_Fire,
	EntityTypeCount
};

struct Entity
{
	s32 index;
	EntityType type;

	Rect2 bounds;
	f32 depth;
	Sprite *sprite;
	V4 color;
};
