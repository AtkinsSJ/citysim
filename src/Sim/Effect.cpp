/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Effect.h"
#include <IO/LineReader.h>

Optional<EffectRadius> EffectRadius::read(LineReader& reader)
{
    Optional radius = readInt<s32>(&reader);

    if (radius.has_value()) {
        Optional centre_value = readInt<s32>(&reader, true);
        Optional outer_value = readInt<s32>(&reader, true);

        return EffectRadius {
            radius.value(),
            centre_value.value_or(radius.value()), // Default to value=radius
            outer_value.value_or(0),
        };
    }

    error(&reader, "Couldn't parse effect radius. Expected \"{0} radius [effectAtCentre] [effectAtEdge]\" where radius, effectAtCentre, and effectAtEdge are ints."_s);

    return {};
}
