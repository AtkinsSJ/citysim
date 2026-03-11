/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Blob.h"
#include <Util/Maths.h>

Blob Blob::sub_blob(size_t start, Optional<size_t> length) const
{
    auto result_length = min(length.value_or(m_size), m_size - start);
    return Blob { result_length, m_data + start };
}
