/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Harness/Harness.h"
#include <Util/HashTable.h>
#include <Util/String.h>

void test_main()
{
    // Basic usage
    {
        HashTable<s8> s8s;
        EXPECT(s8s.count() == 0);
        EXPECT(s8s.contains(1) == false);
        EXPECT(s8s.contains(2) == false);

        // Iteration of an empty HashTable should be valid but produce no values.
        for (auto value : s8s) {
            (void)value;
            EXPECT(false);
        }

        s8s.put(1);
        EXPECT(s8s.count() == 1);
        EXPECT(s8s.contains(1) == true);
        EXPECT(s8s.contains(2) == false);
        s8s.put(1);
        EXPECT(s8s.count() == 1);
        EXPECT(s8s.contains(1) == true);
        EXPECT(s8s.contains(2) == false);
        s8s.remove(1);
        EXPECT(s8s.count() == 0);
        EXPECT(s8s.contains(1) == false);
        EXPECT(s8s.contains(2) == false);

        for (s8 i = 0; i < 10; i++)
            s8s.put(i);
        EXPECT(s8s.count() == 10);

        bool saw_0 = false;
        bool saw_1 = false;
        bool saw_2 = false;
        bool saw_3 = false;
        bool saw_4 = false;
        bool saw_5 = false;
        bool saw_6 = false;
        bool saw_7 = false;
        bool saw_8 = false;
        bool saw_9 = false;

        for (auto value : s8s) {
            switch (value) {
            case 0:
                saw_0 = true;
                break;
            case 1:
                saw_1 = true;
                break;
            case 2:
                saw_2 = true;
                break;
            case 3:
                saw_3 = true;
                break;
            case 4:
                saw_4 = true;
                break;
            case 5:
                saw_5 = true;
                break;
            case 6:
                saw_6 = true;
                break;
            case 7:
                saw_7 = true;
                break;
            case 8:
                saw_8 = true;
                break;
            case 9:
                saw_9 = true;
                break;
            default:
                ASSERT(false);
            }
        }

        EXPECT(saw_0);
        EXPECT(saw_1);
        EXPECT(saw_2);
        EXPECT(saw_3);
        EXPECT(saw_4);
        EXPECT(saw_5);
        EXPECT(saw_6);
        EXPECT(saw_7);
        EXPECT(saw_8);
        EXPECT(saw_9);

        HashTable<s8> others;
        others.put_all(s8s);

        s8s.clear();
        EXPECT(s8s.count() == 0);
        EXPECT(others.count() == 10);
    }

    // Strings
    {
        HashTable<String> strings;
        EXPECT(strings.count() == 0);
        EXPECT(strings.contains("hello"_s) == false);
        EXPECT(strings.contains("world"_s) == false);
        strings.put("hello"_s);
        EXPECT(strings.count() == 1);
        EXPECT(strings.contains("hello"_s) == true);
        EXPECT(strings.contains("world"_s) == false);
        strings.put("hello"_s);
        EXPECT(strings.count() == 1);
        EXPECT(strings.contains("hello"_s) == true);
        EXPECT(strings.contains("world"_s) == false);
        strings.remove("hello"_s);
        EXPECT(strings.count() == 0);
        EXPECT(strings.contains("hello"_s) == false);
        EXPECT(strings.contains("world"_s) == false);
    }
}
