/*
 * Copyright (c) 2025-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Effect.h"

#include <Util/Lexer.h>

Optional<EffectRadius> EffectRadius::read(Lexer& lexer)
{
    return lexer.consume_with_callback<EffectRadius>([](Lexer& lexer) -> Optional<EffectRadius> {
        auto radius = lexer.consume_int<s32>();
        lexer.discard_whitespace();

        if (!radius.has_value())
            return {};

        auto const centre_value = lexer.consume_int<s32>();
        lexer.discard_whitespace();
        auto const outer_value = lexer.consume_int<s32>();
        lexer.discard_whitespace();

        return EffectRadius {
            radius.value(),
            centre_value.value_or(radius.value()), // Default to value=radius
            outer_value.value_or(0),
        };
    });
}
