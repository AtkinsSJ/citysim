/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Harness/Harness.h"
#include <Util/HashTable.h>
#include <Util/String.h>
#include <Util/Variant.h>

void test_main()
{
    struct Thing {
        u8 number { 42 };
        bool operator==(Thing const&) const = default;
    };

    Variant<Empty, float, Thing> variant;

    EXPECT(variant.can_contain<Empty>());
    EXPECT(variant.can_contain<float>());
    EXPECT(variant.can_contain<Thing>());
    EXPECT(!variant.can_contain<u8>());
    EXPECT(!variant.can_contain<Thing*>());

    [[maybe_unused]] auto& empty = variant.get<Empty>();
    EXPECT(variant.has<Empty>());
    EXPECT(!variant.has<float>());
    EXPECT(!variant.has<Thing>());

    variant = 13.37f;
    EXPECT(!variant.has<Empty>());
    EXPECT(variant.has<float>());
    EXPECT(!variant.has<Thing>());
    EXPECT(variant.get<float>() == 13.37f);

    variant = Thing();
    EXPECT(!variant.has<Empty>());
    EXPECT(!variant.has<float>());
    EXPECT(variant.has<Thing>());
    EXPECT(variant.get<Thing>().number == 42);

    variant = Thing(32);
    EXPECT(!variant.has<Empty>());
    EXPECT(!variant.has<float>());
    EXPECT(variant.has<Thing>());
    EXPECT(variant.get<Thing>().number == 32);

    variant = Empty {};
    EXPECT(variant.has<Empty>());
    EXPECT(!variant.has<float>());
    EXPECT(!variant.has<Thing>());

    Variant<Empty, float, Thing> other_variant;
    EXPECT(variant == other_variant);
    other_variant.set(Thing(17));
    EXPECT(variant != other_variant);

    variant.visit([](auto&) { EXPECT(true); });

    variant.visit(
        [](Empty&) { EXPECT(true); },
        [](float&) { EXPECT(false); },
        [](Thing&) { EXPECT(false); });

    other_variant.visit(
        [](Empty&) { EXPECT(false); },
        [](float&) { EXPECT(false); },
        [](Thing&) { EXPECT(true); });

    variant.set(Thing(17));
    EXPECT(variant == other_variant);

    {
        Variant<u8, s32, float, double> all_numbers { 17 };
        EXPECT(!all_numbers.has<u8>());
        EXPECT(all_numbers.has<s32>());
        EXPECT(!all_numbers.has<float>());
        EXPECT(!all_numbers.has<double>());

        all_numbers = (u8)255;
        EXPECT(all_numbers.has<u8>());
        EXPECT(!all_numbers.has<s32>());
        EXPECT(!all_numbers.has<float>());
        EXPECT(!all_numbers.has<double>());

        all_numbers = 13.37;
        EXPECT(!all_numbers.has<u8>());
        EXPECT(!all_numbers.has<s32>());
        EXPECT(!all_numbers.has<float>());
        EXPECT(all_numbers.has<double>());

        all_numbers = 13.37f;
        EXPECT(!all_numbers.has<u8>());
        EXPECT(!all_numbers.has<s32>());
        EXPECT(all_numbers.has<float>());
        EXPECT(!all_numbers.has<double>());
    }

    {
        Variant<int> v { 42 };
        EXPECT(v.get<int>() == 42);
        v.visit([](int& i) { i = 12; });
        EXPECT(v.get<int>() == 12);

        Variant<int> const c { 42 };
        EXPECT(c.get<int>() == 42);
    }

    // Test with movable types.
    {
        HashTable<int> table { 32 };
        table.put("Hello"_s, 42);
        table.put("World"_s, 17);
        Variant<HashTable<int>> table_variant { move(table) };

        // Make sure the HashTable is in there properly
        EXPECT(table_variant.has<HashTable<int>>());
        auto& held_table = table_variant.get<HashTable<int>>();
        {
            auto hello = held_table.findValue("Hello"_s);
            EXPECT(hello.isValid);
            EXPECT(hello.value == 42);
        }
        {
            auto world = held_table.findValue("World"_s);
            EXPECT(world.isValid);
            EXPECT(world.value == 17);
        }
        EXPECT(!held_table.findValue("huh?"_s).isValid);
    }
}
