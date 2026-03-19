/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Budget.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Menus/SaveFile.h>
#include <Sim/City.h>

BudgetLayer::BudgetLayer(City&, MemoryArena&)
{
}

void BudgetLayer::save(BinaryFileWriter& writer) const
{
    writer.startSection<SAVSection_Budget>(SAV_BUDGET_ID, SAV_BUDGET_VERSION);
    SAVSection_Budget budgetSection = {};

    writer.endSection<SAVSection_Budget>(&budgetSection);
}

bool BudgetLayer::load(BinaryFileReader& reader)
{
    bool succeeded = false;
    while (reader.startSection(SAV_BUDGET_ID, SAV_BUDGET_VERSION)) {
        SAVSection_Budget* section = reader.readStruct<SAVSection_Budget>(0);
        if (!section)
            break;

        // TODO: Implement Budget!

        succeeded = true;
        break;
    }

    return succeeded;
}
