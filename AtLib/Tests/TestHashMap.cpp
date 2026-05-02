/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Harness/Harness.h"
#include <Util/HashMap.h>
#include <Util/String.h>

void test_main()
{
    // Basic usage
    {
        HashMap<s8, bool> s8s;
        EXPECT(s8s.count() == 0);
        EXPECT(s8s.contains(1) == false);
        EXPECT(s8s.contains(2) == false);

        // Iteration of an empty HashMap should be valid but produce no values.
        for (auto [key, value] : s8s) {
            (void)key;
            (void)value;
            EXPECT(false);
        }

        s8s.set(1, true);
        EXPECT(s8s.count() == 1);
        EXPECT(s8s.contains(1) == true);
        EXPECT(s8s.contains(2) == false);
        s8s.set(1, true);
        EXPECT(s8s.count() == 1);
        EXPECT(s8s.contains(1) == true);
        EXPECT(s8s.contains(2) == false);
        s8s.remove(1);
        EXPECT(s8s.count() == 0);
        EXPECT(s8s.contains(1) == false);
        EXPECT(s8s.contains(2) == false);

        for (s8 i = 0; i < 10; i++)
            s8s.set(i, true);
        EXPECT(s8s.count() == 10);

        Optional<bool> saw_0;
        Optional<bool> saw_1;
        Optional<bool> saw_2;
        Optional<bool> saw_3;
        Optional<bool> saw_4;
        Optional<bool> saw_5;
        Optional<bool> saw_6;
        Optional<bool> saw_7;
        Optional<bool> saw_8;
        Optional<bool> saw_9;

        for (auto [key, value] : s8s) {
            switch (key) {
            case 0:
                saw_0 = value;
                break;
            case 1:
                saw_1 = value;
                break;
            case 2:
                saw_2 = value;
                break;
            case 3:
                saw_3 = value;
                break;
            case 4:
                saw_4 = value;
                break;
            case 5:
                saw_5 = value;
                break;
            case 6:
                saw_6 = value;
                break;
            case 7:
                saw_7 = value;
                break;
            case 8:
                saw_8 = value;
                break;
            case 9:
                saw_9 = value;
                break;
            default:
                ASSERT(false);
            }
        }

        EXPECT(saw_0 == true);
        EXPECT(saw_1 == true);
        EXPECT(saw_2 == true);
        EXPECT(saw_3 == true);
        EXPECT(saw_4 == true);
        EXPECT(saw_5 == true);
        EXPECT(saw_6 == true);
        EXPECT(saw_7 == true);
        EXPECT(saw_8 == true);
        EXPECT(saw_9 == true);
    }

    // Strings
    {
        HashMap<String, u32> strings;
        EXPECT(strings.count() == 0);
        EXPECT(strings.contains("hello"_s) == false);
        EXPECT(strings.contains("world"_s) == false);
        strings.set("hello"_s, 7);
        EXPECT(strings.count() == 1);
        EXPECT(strings.contains("hello"_s) == true);
        EXPECT(strings.contains("world"_s) == false);
        EXPECT(strings.get("hello"_s) == 7);
        strings.set("hello"_s, 23);
        EXPECT(strings.count() == 1);
        EXPECT(strings.contains("hello"_s) == true);
        EXPECT(strings.contains("world"_s) == false);
        strings.remove("hello"_s);
        EXPECT(strings.count() == 0);
        EXPECT(strings.contains("hello"_s) == false);
        EXPECT(strings.contains("world"_s) == false);
    }
}
