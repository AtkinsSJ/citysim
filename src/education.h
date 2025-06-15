/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Forward.h>

struct City;

struct EducationLayer {
};

void initEducationLayer(EducationLayer* layer, City* city, MemoryArena* gameArena);
void saveEducationLayer(EducationLayer* layer, struct BinaryFileWriter* writer);
bool loadEducationLayer(EducationLayer* layer, City* city, struct BinaryFileReader* reader);
