/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Cursor.h"
#include <Assets/AssetManager.h>

Cursor& Cursor::get(StringView name)
{
    return getAsset(AssetType::Cursor, name.deprecated_to_string()).cursor;
}
