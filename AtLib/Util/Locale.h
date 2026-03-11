/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Forward.h>
#include <Util/Optional.h>

enum class Locale : u8 {
    En,
    Es,
    Pl,
    COUNT
};

Optional<Locale> locale_from_string(String locale);
String to_string(Locale);
