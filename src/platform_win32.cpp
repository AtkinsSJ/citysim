/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "platform.h"
#define NOMINMAX
#include <shellapi.h>
#include <windows.h>

void openUrlUnsafe(char const* url)
{
    ShellExecute(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
}
