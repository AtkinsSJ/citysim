/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Characters.h>
#include <Util/Function.h>
#include <Util/StringView.h>

class Lexer {
public:
    explicit Lexer(StringView input);

    bool has_next() const;

    // FIXME: It's likely that we'll want to make this unicode-safe later, but right now I just want it for ASCII-only
    //        data files. Sorry future me! Hopefully it won't be too hard to adapt.
    Optional<char> peek() const;
    Optional<char> consume();
    bool consume_specific(char);
    bool consume_specific(StringView);
    Optional<StringView> consume_until(char end);
    Optional<StringView> consume_until(Function<bool(char)> const&);
    Optional<StringView> consume_while(Function<bool(char)> const&);

    template<typename Result, typename Callback>
    Optional<Result> consume_with_callback(Callback const& callback)
    {
        auto const old_position = m_position;
        auto result = callback(*this);
        if (result.has_value())
            return result;
        m_position = old_position;
        return {};
    }

    // More specific functions
    void discard_whitespace();
    Optional<StringView> consume_token();

    template<Integral I>
    Optional<I> consume_int()
    {
        return consume_with_callback<I>([](Lexer& lexer) -> Optional<I> {
            bool is_negative = false;
            if constexpr (IsSigned<I>) {
                is_negative = lexer.consume_specific('-');
            }
            // Allow a + sign, which does nothing.
            if (!is_negative)
                lexer.consume_specific('+');

            u64 value = 0;
            auto digits = lexer.consume_while([](char c) { return is_ascii_numeric(c); });
            if (!digits.has_value())
                return {};

            // NB: We allow leading 0s.
            // FIXME: Char iterator on StringBase!
            for (char c : digits.value().bytes()) {
                if (c < '0' || c > '9')
                    return {};

                u64 const new_value = value * 10 + (c - '0');
                if (new_value < value) {
                    // We overflowed!
                    return {};
                }
                value = new_value;
            }

            if constexpr (IsSigned<I>) {
                s64 signed_value = is_negative ? -value : value;
                if (signed_value < MinValue<I> || signed_value > MaxValue<I>)
                    return {};
                // Checking that the signed value is in the range above doesn't work for the min and max values for s64,
                // because we can over- or underflow and the value will still be in range! So instead, ensure that if we
                // expected a negative number, it's negative, and if we expected a positive one, it is.
                if (is_negative && signed_value > 0)
                    return {};
                if (!is_negative && signed_value < 0)
                    return {};
                return static_cast<I>(signed_value);
            }

            if (value < MinValue<I> || value > MaxValue<I>)
                return {};
            return static_cast<I>(value);
        });
    }

    template<FloatingPoint F>
    Optional<F> consume_float()
    {
        return consume_with_callback<F>([](Lexer& lexer) -> Optional<F> {
            // Specifically parses decimal notation, not scientific or numbers like NaN or Infinity.
            // Sign is optional. Fractional part is optional, but if the '.' is present then the fractional part must be too.

            auto maybe_whole_component = lexer.consume_int<s64>();
            if (!maybe_whole_component.has_value())
                return {};

            s64 whole_component = maybe_whole_component.release_value();
            s64 fractional_component = 0;
            s64 fractional_component_length = 0;
            if (lexer.consume_specific('.')) {
                auto maybe_fraction_string = lexer.consume_while([](auto c) { return is_ascii_numeric(c); });
                if (!maybe_fraction_string.has_value())
                    return {};
                fractional_component_length = maybe_fraction_string.value().length();
                auto maybe_fractional_part = maybe_fraction_string.value().to_int();
                if (!maybe_fractional_part.has_value())
                    return {};
                fractional_component = maybe_fractional_part.release_value();
            }

            double value = static_cast<double>(whole_component);
            if (fractional_component_length > 0) {
                value += static_cast<double>(fractional_component) / powf64(10.0, static_cast<double>(fractional_component_length));
            }

            // NB: This copies the old behaviour of allowing percentages and interpreting 100% as 1.0.
            if (lexer.consume_specific('%'))
                return static_cast<F>(value * 0.01);

            return static_cast<F>(value);
        });
    }

    Optional<bool> consume_bool();

private:
    StringView m_input;
    u64 m_position = 0;
};
