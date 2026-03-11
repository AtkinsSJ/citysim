/*
 * Copyright (c) 2019-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Alignment.h"
#include <IO/LineReader.h>

Optional<Alignment> Alignment::read(LineReader& reader)
{
    Optional<HAlign> h;
    Optional<VAlign> v;

    auto token = reader.next_token();
    if (!token.has_value()) {
        reader.error("Expected alignment keywords"_s);
        return {};
    }

    while (token.has_value()) {
        if (token == "LEFT"_s) {
            if (h.has_value()) {
                reader.error("Multiple horizontal alignment keywords given!"_s);
                break;
            }
            h = HAlign::Left;
        } else if (token == "H_CENTRE"_s) {
            if (h.has_value()) {
                reader.error("Multiple horizontal alignment keywords given!"_s);
                break;
            }
            h = HAlign::Centre;
        } else if (token == "RIGHT"_s) {
            if (h.has_value()) {
                reader.error("Multiple horizontal alignment keywords given!"_s);
                break;
            }
            h = HAlign::Right;
        } else if (token == "EXPAND_H"_s) {
            if (h.has_value()) {
                reader.error("Multiple horizontal alignment keywords given!"_s);
                break;
            }
            h = HAlign::Fill;
        } else if (token == "TOP"_s) {
            if (v.has_value()) {
                reader.error("Multiple vertical alignment keywords given!"_s);
                break;
            }
            v = VAlign::Top;
        } else if (token == "V_CENTRE"_s) {
            if (v.has_value()) {
                reader.error("Multiple vertical alignment keywords given!"_s);
                break;
            }
            v = VAlign::Centre;
        } else if (token == "BOTTOM"_s) {
            if (v.has_value()) {
                reader.error("Multiple vertical alignment keywords given!"_s);
                break;
            }
            v = VAlign::Bottom;
        } else if (token == "EXPAND_V"_s) {
            if (v.has_value()) {
                reader.error("Multiple vertical alignment keywords given!"_s);
                break;
            }
            v = VAlign::Fill;
        } else {
            reader.error("Unrecognized alignment keyword '{0}'"_s, { token.value() });

            return {};
        }

        token = reader.next_token();
    }

    return Alignment { h.release_value(), v.release_value() };
}
