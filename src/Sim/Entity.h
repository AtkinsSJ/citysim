/*
 * Copyright (c) 2020-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Gfx/Colour.h>
#include <Gfx/Sprite.h>
#include <Util/Basic.h>
#include <Util/Rectangle.h>

struct Entity {
    enum class Type : u8 {
        None,
        Building,
        Fire,
        COUNT,
    };

    s32 index;
    Type type;
    void* dataPointer; // Type-checked to match the `type`, see checkEntityMatchesType()

    // Drawing data
    Rect2 bounds;
    float depth;
    SpriteRef sprite;
    Colour color;

    // Misc
    bool canBeDemolished; // TODO: make this be flags?
};

template<typename T>
inline bool checkEntityMatchesType(Entity* entity)
{
    // TODO: Re-enable this. We currently can't call it because the structs (Building, Fire) aren't included yet.
    return true;

    /*
            // I'm expecting typos where the Entity::Type and data pointer provided or requested don't match,
            // so here's a little function to check that.
            switch (entity->type)
            {
                    case Entity::Type::Building: return (typeid(T) == typeid(struct Building));
                    case Entity::Type::Fire: 	  return (typeid(T) == typeid(struct Fire));
                    INVALID_DEFAULT_CASE;
            }

            return false;
    */
}

template<typename T>
inline T* getEntityData(Entity* entity)
{
    ASSERT(checkEntityMatchesType<T>(entity));

    return (T*)entity->dataPointer;
}
