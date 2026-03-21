/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Sim/Forward.h>
#include <Util/Rectangle.h>

class Layer {
public:
    virtual ~Layer() = default;

    virtual void update(City&) { }
    virtual void mark_dirty(Rect2I bounds) { }

    virtual void notify_new_building(BuildingDef const&, Building&) { }
    virtual void notify_building_demolished(BuildingDef const&, Building&) { }

    virtual void save(BinaryFileWriter&) const = 0;
    virtual bool load(BinaryFileReader&, City&) = 0;
};
