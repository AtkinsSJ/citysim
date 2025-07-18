/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "building.h"
#include "city.h"
#include <Sim/BuildingRef.h>

BuildingRef getReferenceTo(Building* building)
{
    BuildingRef result = {};
    result.buildingID = building->id;
    result.buildingPos = building->footprint.pos;

    return result;
}

Building* getBuilding(City* city, BuildingRef ref)
{
    Building* result = nullptr;

    Building* buildingAtPosition = getBuildingAt(city, ref.buildingPos.x, ref.buildingPos.y);
    if ((buildingAtPosition != nullptr) && (buildingAtPosition->id == ref.buildingID)) {
        result = buildingAtPosition;
    }

    return result;
}
