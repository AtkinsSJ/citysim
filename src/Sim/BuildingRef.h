/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Sim/Forward.h>
#include <Util/Basic.h>
#include <Util/Vector.h>

// NB: If someone needs a pointer to a building across multiple frames, use one of these references.
// The position lets you look-up the building via getBuildingAt(), and the buildingID lets you check
// that the found building is indeed the one you were after.
// You'd then do something like this:
//     Building *theBuilding = getBuilding(city, buildingRef);
// which would look-up the building, check its ID, and return the Building* if it matches
// and null if it doesn't, or if no building is at the position any more.
// - Sam, 19/08/2019
struct BuildingRef {
    u32 buildingID;
    V2I buildingPos;

    bool operator==(BuildingRef const&) const = default;
};
BuildingRef getReferenceTo(Building* building);
Building* getBuilding(City* city, BuildingRef ref);
