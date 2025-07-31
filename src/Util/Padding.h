/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <IO/Forward.h>
#include <Util/Basic.h>
#include <Util/Forward.h>

struct Padding {
    s32 top;
    s32 bottom;
    s32 left;
    s32 right;

    static Optional<Padding> read(LineReader&);
};
