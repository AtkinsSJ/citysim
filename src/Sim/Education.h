/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Sim/Forward.h>
#include <Util/Forward.h>

struct EducationLayer {
};

void initEducationLayer(EducationLayer* layer, City* city, MemoryArena* gameArena);
void saveEducationLayer(EducationLayer* layer, BinaryFileWriter* writer);
bool loadEducationLayer(EducationLayer* layer, City* city, BinaryFileReader* reader);
