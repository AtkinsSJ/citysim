/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Effect.h"
#include <IO/LineReader.h>

ErrorOr<EffectRadius> EffectRadius::read(LineReader& reader)
{
    auto radius = reader.read_int<s32>();

    if (radius.has_value()) {
        auto centre_value = reader.read_int<s32>(LineReader::IsRequired::No);
        auto outer_value = reader.read_int<s32>(LineReader::IsRequired::No);

        return EffectRadius {
            radius.value(),
            centre_value.value_or(radius.value()), // Default to value=radius
            outer_value.value_or(0),
        };
    }

    return reader.make_error_message("Couldn't parse effect radius. Expected \"radius [effectAtCentre] [effectAtEdge]\" where radius, effectAtCentre, and effectAtEdge are ints."_s);
}
