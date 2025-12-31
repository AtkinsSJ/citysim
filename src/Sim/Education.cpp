/*
 * Copyright (c) 2021-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Education.h"
#include <IO/BinaryFileReader.h>
#include <IO/BinaryFileWriter.h>
#include <IO/SaveFile.h>

void initEducationLayer(EducationLayer* layer, City*, MemoryArena*)
{
    *layer = {};
}

void saveEducationLayer(EducationLayer*, BinaryFileWriter* writer)
{
    writer->startSection<SAVSection_Education>(SAV_EDUCATION_ID, SAV_EDUCATION_VERSION);
    SAVSection_Education educationSection = {};

    writer->endSection<SAVSection_Education>(&educationSection);
}

bool loadEducationLayer(EducationLayer*, City*, BinaryFileReader* reader)
{
    bool succeeded = false;
    while (reader->startSection(SAV_EDUCATION_ID, SAV_EDUCATION_VERSION)) {
        SAVSection_Education* section = reader->readStruct<SAVSection_Education>(0);
        if (!section)
            break;

        succeeded = true;
        break;
    }

    return succeeded;
}
