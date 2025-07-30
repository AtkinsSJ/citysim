/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Budget.h"
#include "../save_file.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Sim/City.h>

void initBudgetLayer(BudgetLayer* layer, City*, MemoryArena*)
{
    *layer = {};
}

void saveBudgetLayer(BudgetLayer*, BinaryFileWriter* writer)
{
    writer->startSection<SAVSection_Budget>(SAV_BUDGET_ID, SAV_BUDGET_VERSION);
    SAVSection_Budget budgetSection = {};

    writer->endSection<SAVSection_Budget>(&budgetSection);
}

bool loadBudgetLayer(BudgetLayer*, City*, BinaryFileReader* reader)
{
    bool succeeded = false;
    while (reader->startSection(SAV_BUDGET_ID, SAV_BUDGET_VERSION)) {
        SAVSection_Budget* section = reader->readStruct<SAVSection_Budget>(0);
        if (!section)
            break;

        // TODO: Implement Budget!

        succeeded = true;
        break;
    }

    return succeeded;
}
