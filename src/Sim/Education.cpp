/*
 * Copyright (c) 2021-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Education.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <Menus/SaveFile.h>

EducationLayer::EducationLayer(City&, MemoryArena&)
{
}

void EducationLayer::save(BinaryFileWriter& writer) const
{
    writer.startSection<SAVSection_Education>(SAV_EDUCATION_ID, SAV_EDUCATION_VERSION);
    SAVSection_Education educationSection = {};

    writer.endSection<SAVSection_Education>(&educationSection);
}

bool EducationLayer::load(BinaryFileReader& reader, City&)
{
    bool succeeded = false;
    while (reader.startSection(SAV_EDUCATION_ID, SAV_EDUCATION_VERSION)) {
        SAVSection_Education* section = reader.readStruct<SAVSection_Education>(0);
        if (!section)
            break;

        succeeded = true;
        break;
    }

    return succeeded;
}
