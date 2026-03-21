/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/Forward.h>
#include <Sim/Layer.h>
#include <Util/Forward.h>

class BudgetLayer final : public Layer {
public:
    BudgetLayer() = default;
    BudgetLayer(City&, MemoryArena&);
    virtual ~BudgetLayer() override = default;

    virtual void save(BinaryFileWriter&) const override;
    virtual bool load(BinaryFileReader&, City&) override;
};
