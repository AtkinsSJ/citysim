/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Padding.h"
#include <IO/LineReader.h>

Optional<Padding> Padding::read(LineReader& reader)
{
    // Padding definitions may be 1, 2, 3 or 4 values, as CSS does it:
    //   All
    //   Vertical Horizontal
    //   Top Horizontal Bottom
    //   Top Right Bottom Left
    // So, clockwise from the top, with sensible fallbacks

    switch (reader.count_remaining_tokens_in_current_line()) {
    case 1: {
        if (auto value = reader.read_int<s32>(); value.has_value()) {
            return Padding {
                .top = value.value(),
                .bottom = value.value(),
                .left = value.value(),
                .right = value.value(),
            };
        }
    } break;

    case 2: {
        auto v_value = reader.read_int<s32>();
        auto h_value = reader.read_int<s32>();
        if (v_value.has_value() && h_value.has_value()) {
            return Padding {
                .top = v_value.value(),
                .bottom = v_value.value(),
                .left = h_value.value(),
                .right = h_value.value(),
            };
        }
    } break;

    case 3: {
        auto top_value = reader.read_int<s32>();
        auto h_value = reader.read_int<s32>();
        auto bottom_value = reader.read_int<s32>();
        if (top_value.has_value() && h_value.has_value() && bottom_value.has_value()) {
            return Padding {
                .top = top_value.value(),
                .bottom = bottom_value.value(),
                .left = h_value.value(),
                .right = h_value.value(),
            };
        }
    } break;

    case 4: {
        auto top_value = reader.read_int<s32>();
        auto right_value = reader.read_int<s32>();
        auto bottom_value = reader.read_int<s32>();
        auto left_value = reader.read_int<s32>();
        if (all_have_values(top_value, right_value, bottom_value, left_value)) {
            return Padding {
                .top = top_value.value(),
                .bottom = bottom_value.value(),
                .left = left_value.value(),
                .right = right_value.value(),
            };
        }
    } break;

    default:
        break;
    }

    return {};
}
