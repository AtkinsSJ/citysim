#pragma once

void updateEntity(City *city, Entity *entity)
{
	switch (entity->type)
	{
		case EntityType_Building: updateBuilding(city, getEntityData<Building>(entity));
		case EntityType_Fire: 	  updateFire    (city, getEntityData<Fire>(entity));
		INVALID_DEFAULT_CASE;
	}
}
