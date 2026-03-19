/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/Forward.h>
#include <Util/Forward.h>

class BudgetLayer {
public:
    BudgetLayer() = default;
    BudgetLayer(City&, MemoryArena&);

    void save(BinaryFileWriter&) const;
    bool load(BinaryFileReader&);
};
