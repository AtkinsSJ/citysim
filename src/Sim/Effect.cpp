/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Effect.h"
#include <IO/LineReader.h>

Optional<EffectRadius> EffectRadius::read(LineReader& reader)
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

    reader.error("Couldn't parse effect radius. Expected \"{0} radius [effectAtCentre] [effectAtEdge]\" where radius, effectAtCentre, and effectAtEdge are ints."_s);

    return {};
}
