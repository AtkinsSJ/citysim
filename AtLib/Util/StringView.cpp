/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "StringView.h"
#include <Util/String.h>

String StringView::deprecated_to_string() const
{
    return String { raw_pointer_to_characters(), length() };
}
