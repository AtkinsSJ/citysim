/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Sim/Forward.h>
#include <Util/MemoryArena.h>

struct BudgetLayer {
};

void initBudgetLayer(BudgetLayer* layer, City* city, MemoryArena* gameArena);
void saveBudgetLayer(BudgetLayer* layer, struct BinaryFileWriter* writer);
bool loadBudgetLayer(BudgetLayer* layer, City* city, struct BinaryFileReader* reader);
