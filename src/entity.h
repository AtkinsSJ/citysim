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
	void *dataPointer; // Type-checked to match the `type`, see checkEntityMatchesType()

	// Drawing data
	Rect2 bounds;
	f32 depth;
	Sprite *sprite;
	V4 color;
};

template <typename T>
inline T* getEntityData(Entity *entity)
{
	ASSERT(checkEntityMatchesType<T>(entity));

	return (T*) entity->dataPointer;
}

template <typename T>
inline bool checkEntityMatchesType(Entity *entity)
{
	// I'm expecting typos where the EntityType and data pointer provided or requested don't match,
	// so here's a little function to check that.
	switch (entity->type)
	{
		case EntityType_Building: return (typeid(T*) == typeid(Building*));
		case EntityType_Fire: 	  return (typeid(T*) == typeid(Fire*));
		INVALID_DEFAULT_CASE;
	}

	return false;
}

void updateEntity(City *city, Entity *entity);
