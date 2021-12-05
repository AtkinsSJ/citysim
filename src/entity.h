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
	SpriteRef sprite;
	V4 color;

	// Misc
	bool canBeDemolished; // TODO: make this be flags?
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
	// TODO: Re-enable this. We currently can't call it because the structs (Building, Fire) aren't included yet.
	return true;

/*
	// I'm expecting typos where the EntityType and data pointer provided or requested don't match,
	// so here's a little function to check that.
	switch (entity->type)
	{
		case EntityType_Building: return (typeid(T) == typeid(struct Building));
		case EntityType_Fire: 	  return (typeid(T) == typeid(struct Fire));
		INVALID_DEFAULT_CASE;
	}

	return false;
*/
}
