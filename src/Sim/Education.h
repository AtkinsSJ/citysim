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

class EducationLayer final : public Layer {
public:
    EducationLayer() = default;
    EducationLayer(City&, MemoryArena&);
    virtual ~EducationLayer() override = default;

    virtual void save(BinaryFileWriter&) const override;
    virtual bool load(BinaryFileReader&, City&) override;
};
