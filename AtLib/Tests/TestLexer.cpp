/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Harness/Harness.h"
#include <Util/Lexer.h>

void test_main()
{
    // Blank input
    {
        Lexer lexer { ""_sv };
        EXPECT(!lexer.has_next());
        EXPECT(!lexer.peek().has_value());
        EXPECT(!lexer.consume().has_value());
        EXPECT(!lexer.consume_specific(' '));
        EXPECT(!lexer.consume_until(' ').has_value());
    }

    // Peek and consume
    {
        Lexer lexer { "flogwizzle"_sv };
        EXPECT(lexer.has_next());
        EXPECT(lexer.peek() == 'f');
        EXPECT(lexer.peek() == 'f');
        EXPECT(lexer.peek() == 'f');
        EXPECT(lexer.consume() == 'f');
        EXPECT(lexer.consume_specific("logwizzl"_sv));
        EXPECT(lexer.peek() == 'e');
        EXPECT(lexer.peek() == 'e');
        EXPECT(lexer.peek() == 'e');
        EXPECT(lexer.consume() == 'e');
        EXPECT(!lexer.peek().has_value());
        EXPECT(!lexer.consume().has_value());
        EXPECT(!lexer.has_next());
    }

    // Consume specific
    {
        Lexer lexer { "flogwizzle"_sv };
        EXPECT(lexer.has_next());
        EXPECT(lexer.consume_specific('f'));
        EXPECT(lexer.consume_specific("logwizzl"_sv));
        EXPECT(lexer.consume_specific('e'));
        EXPECT(!lexer.has_next());
    }

    // Consume until
    {
        Lexer lexer { "flogwizzle"_sv };
        EXPECT(lexer.has_next());
        EXPECT(lexer.consume_until('w') == "flog"_sv);
        EXPECT(lexer.consume_specific('w'));
        EXPECT(lexer.consume_until('w') == "izzle"_sv);
        EXPECT(!lexer.has_next());
    }

    // Consume until lambda
    {
        Lexer lexer { "flogwizzle"_sv };
        EXPECT(lexer.has_next());
        EXPECT(lexer.consume_until([](char c) { return c >= 't'; }) == "flog"_sv);
        EXPECT(!lexer.consume_until([](auto) { return true; }).has_value());
        EXPECT(lexer.consume_until([](auto) { return false; }) == "wizzle"_sv);
        EXPECT(!lexer.has_next());
    }

    // Consume while
    {
        Lexer lexer { "flogwizzle"_sv };
        EXPECT(lexer.has_next());
        EXPECT(lexer.consume_while([](char c) { return c < 't'; }) == "flog"_sv);
        EXPECT(!lexer.consume_while([](auto) { return false; }).has_value());
        EXPECT(lexer.consume_while([](auto) { return true; }) == "wizzle"_sv);
        EXPECT(!lexer.has_next());
    }

    // Whitespace and tokens
    {
        Lexer lexer { " foo     bar\t\t\tbaz       1+2=lol   "_sv };
        EXPECT(lexer.has_next());
        EXPECT(lexer.consume_token().has_value() == false);
        lexer.discard_whitespace();
        EXPECT(lexer.consume_token() == "foo"_sv);
        lexer.discard_whitespace();
        EXPECT(lexer.consume_token() == "bar"_sv);
        lexer.discard_whitespace();
        EXPECT(lexer.consume_token() == "baz"_sv);
        lexer.discard_whitespace();
        EXPECT(lexer.consume_token() == "1+2=lol"_sv);
        EXPECT(lexer.has_next());
        lexer.discard_whitespace();
        EXPECT(lexer.has_next() == false);
        EXPECT(lexer.consume_token().has_value() == false);
    }

    // Integers
    {
        // Test a scattering of integer values, for reasonable coverage:
        //    0, +0, -0
        //    1, +1, -1
        //    min, max
        //    min-1, max+1

        EXPECT(Lexer { "0"_sv }.consume_int<u8>() == 0);
        EXPECT(Lexer { "+0"_sv }.consume_int<u8>() == 0);
        EXPECT(Lexer { "-0"_sv }.consume_int<u8>().has_value() == false);
        EXPECT(Lexer { "1"_sv }.consume_int<u8>() == 1);
        EXPECT(Lexer { "+1"_sv }.consume_int<u8>() == 1);
        EXPECT(Lexer { "-1"_sv }.consume_int<u8>().has_value() == false);
        EXPECT(Lexer { "255"_sv }.consume_int<u8>() == MaxValue<u8>);
        EXPECT(Lexer { "256"_sv }.consume_int<u8>().has_value() == false);

        EXPECT(Lexer { "0"_sv }.consume_int<u16>() == 0);
        EXPECT(Lexer { "+0"_sv }.consume_int<u16>() == 0);
        EXPECT(Lexer { "-0"_sv }.consume_int<u16>().has_value() == false);
        EXPECT(Lexer { "1"_sv }.consume_int<u16>() == 1);
        EXPECT(Lexer { "+1"_sv }.consume_int<u16>() == 1);
        EXPECT(Lexer { "-1"_sv }.consume_int<u16>().has_value() == false);
        EXPECT(Lexer { "65535"_sv }.consume_int<u16>() == MaxValue<u16>);
        EXPECT(Lexer { "65536"_sv }.consume_int<u16>().has_value() == false);

        EXPECT(Lexer { "0"_sv }.consume_int<u32>() == 0);
        EXPECT(Lexer { "+0"_sv }.consume_int<u32>() == 0);
        EXPECT(Lexer { "-0"_sv }.consume_int<u32>().has_value() == false);
        EXPECT(Lexer { "1"_sv }.consume_int<u32>() == 1);
        EXPECT(Lexer { "+1"_sv }.consume_int<u32>() == 1);
        EXPECT(Lexer { "-1"_sv }.consume_int<u32>().has_value() == false);
        EXPECT(Lexer { "4294967295"_sv }.consume_int<u32>() == MaxValue<u32>);
        EXPECT(Lexer { "4294967296"_sv }.consume_int<u32>().has_value() == false);

        EXPECT(Lexer { "0"_sv }.consume_int<u64>() == 0);
        EXPECT(Lexer { "+0"_sv }.consume_int<u64>() == 0);
        EXPECT(Lexer { "-0"_sv }.consume_int<u64>().has_value() == false);
        EXPECT(Lexer { "1"_sv }.consume_int<u64>() == 1);
        EXPECT(Lexer { "+1"_sv }.consume_int<u64>() == 1);
        EXPECT(Lexer { "-1"_sv }.consume_int<u64>().has_value() == false);
        EXPECT(Lexer { "18446744073709551615"_sv }.consume_int<u64>() == MaxValue<u64>);
        EXPECT(Lexer { "18446744073709551616"_sv }.consume_int<u64>().has_value() == false);

        EXPECT(Lexer { "0"_sv }.consume_int<s8>() == 0);
        EXPECT(Lexer { "+0"_sv }.consume_int<s8>() == 0);
        EXPECT(Lexer { "-0"_sv }.consume_int<s8>() == 0);
        EXPECT(Lexer { "1"_sv }.consume_int<s8>() == 1);
        EXPECT(Lexer { "+1"_sv }.consume_int<s8>() == 1);
        EXPECT(Lexer { "-1"_sv }.consume_int<s8>() == -1);
        EXPECT(Lexer { "-128"_sv }.consume_int<s8>() == MinValue<s8>);
        EXPECT(Lexer { "-129"_sv }.consume_int<s8>().has_value() == false);
        EXPECT(Lexer { "127"_sv }.consume_int<s8>() == MaxValue<s8>);
        EXPECT(Lexer { "128"_sv }.consume_int<s8>().has_value() == false);

        EXPECT(Lexer { "0"_sv }.consume_int<s16>() == 0);
        EXPECT(Lexer { "+0"_sv }.consume_int<s16>() == 0);
        EXPECT(Lexer { "-0"_sv }.consume_int<s16>() == 0);
        EXPECT(Lexer { "1"_sv }.consume_int<s16>() == 1);
        EXPECT(Lexer { "+1"_sv }.consume_int<s16>() == 1);
        EXPECT(Lexer { "-1"_sv }.consume_int<s16>() == -1);
        EXPECT(Lexer { "-32768"_sv }.consume_int<s16>() == MinValue<s16>);
        EXPECT(Lexer { "-32769"_sv }.consume_int<s16>().has_value() == false);
        EXPECT(Lexer { "32767"_sv }.consume_int<s16>() == MaxValue<s16>);
        EXPECT(Lexer { "32768"_sv }.consume_int<s16>().has_value() == false);

        EXPECT(Lexer { "0"_sv }.consume_int<s32>() == 0);
        EXPECT(Lexer { "+0"_sv }.consume_int<s32>() == 0);
        EXPECT(Lexer { "-0"_sv }.consume_int<s32>() == 0);
        EXPECT(Lexer { "1"_sv }.consume_int<s32>() == 1);
        EXPECT(Lexer { "+1"_sv }.consume_int<s32>() == 1);
        EXPECT(Lexer { "-1"_sv }.consume_int<s32>() == -1);
        EXPECT(Lexer { "-2147483648"_sv }.consume_int<s32>() == MinValue<s32>);
        EXPECT(Lexer { "-2147483649"_sv }.consume_int<s32>().has_value() == false);
        EXPECT(Lexer { "2147483647"_sv }.consume_int<s32>() == MaxValue<s32>);
        EXPECT(Lexer { "2147483648"_sv }.consume_int<s32>().has_value() == false);

        EXPECT(Lexer { "0"_sv }.consume_int<s64>() == 0);
        EXPECT(Lexer { "+0"_sv }.consume_int<s64>() == 0);
        EXPECT(Lexer { "-0"_sv }.consume_int<s64>() == 0);
        EXPECT(Lexer { "1"_sv }.consume_int<s64>() == 1);
        EXPECT(Lexer { "+1"_sv }.consume_int<s64>() == 1);
        EXPECT(Lexer { "-1"_sv }.consume_int<s64>() == -1);
        EXPECT(Lexer { "-9223372036854775808"_sv }.consume_int<s64>() == MinValue<s64>);
        EXPECT(Lexer { "-9223372036854775809"_sv }.consume_int<s64>().has_value() == false);
        EXPECT(Lexer { "9223372036854775807"_sv }.consume_int<s64>() == MaxValue<s64>);
        EXPECT(Lexer { "9223372036854775808"_sv }.consume_int<s64>().has_value() == false);

        // And now some quick checks for integers between other text
        Lexer lexer { "foo17bar"_sv };
        EXPECT(lexer.consume_int<s64>().has_value() == false);
        EXPECT(lexer.consume_specific("foo"_sv));
        EXPECT(lexer.consume_int<s64>() == 17);
        EXPECT(lexer.consume_specific("bar"_sv));
        EXPECT(lexer.has_next() == false);
    }

    // Floating point things
    // FIXME: The test reporting here isn't great, it points at the lambda's lines and doesn't output the original test's text.
    {
        auto test_lexing_float_success = [](StringView const input, float const expected) {
            printf("Testing float: \"%s\" should equal `%f`\n", input.raw_pointer_to_characters(), expected);
            Lexer lexer { input };
            auto result = lexer.consume_float<float>();
            EXPECT(result.has_value());
            EXPECT(equals_with_epsilon(result.value(), expected, 0.0001f));
            EXPECT(!lexer.has_next());
        };
        test_lexing_float_success("0"_sv, 0.0f);
        test_lexing_float_success("0.0"_sv, 0.0f);
        test_lexing_float_success("-999"_sv, -999.0f);
        test_lexing_float_success("-999.0"_sv, -999.0f);
        test_lexing_float_success("+123456.789"_sv, 123456.789f);
    }
    // Same for doubles
    {
        auto test_lexing_double_success = [](StringView const input, double const expected) {
            printf("Testing double: \"%s\" should equal `%f`\n", input.raw_pointer_to_characters(), expected);
            Lexer lexer { input };
            auto result = lexer.consume_float<double>();
            EXPECT(result.has_value());
            EXPECT(equals_with_epsilon(result.value(), expected, 0.00001));
            EXPECT(!lexer.has_next());
        };
        test_lexing_double_success("0"_sv, 0.0);
        test_lexing_double_success("0.0"_sv, 0.0);
        test_lexing_double_success("-999"_sv, -999.0);
        test_lexing_double_success("-999.0"_sv, -999.0);
        test_lexing_double_success("+123456.789"_sv, 123456.789);
    }
}
