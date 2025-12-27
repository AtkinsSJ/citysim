/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "platform.h"
#include <IO/File.h>
#include <ctime>
#include <dirent.h>

void openUrlUnsafe(char const* url)
{
    char const* format = "xdg-open '%s'";
    smm totalSize = strlen(format) + strlen(url);
    char* buffer = new char[totalSize];
    snprintf(buffer, sizeof(buffer), format, url);
    system(buffer);
    delete[] buffer;
}
