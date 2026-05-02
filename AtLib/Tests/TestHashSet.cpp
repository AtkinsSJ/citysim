/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Harness/Harness.h"
#include <Util/HashSet.h>
#include <Util/String.h>

void test_main()
{
    // Basic usage
    {
        HashSet<s8> s8s;
        EXPECT(s8s.count() == 0);
        EXPECT(s8s.contains(1) == false);
        EXPECT(s8s.contains(2) == false);

        // Iteration of an empty HashSet should be valid but produce no values.
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

        for (s8 i = 0; i < 10; i++) {
            s8s.put(i);
            s8s.put(i);
            s8s.put(i);
        }
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
                ASSERT(saw_0 == false);
                saw_0 = true;
                break;
            case 1:
                ASSERT(saw_1 == false);
                saw_1 = true;
                break;
            case 2:
                ASSERT(saw_2 == false);
                saw_2 = true;
                break;
            case 3:
                ASSERT(saw_3 == false);
                saw_3 = true;
                break;
            case 4:
                ASSERT(saw_4 == false);
                saw_4 = true;
                break;
            case 5:
                ASSERT(saw_5 == false);
                saw_5 = true;
                break;
            case 6:
                ASSERT(saw_6 == false);
                saw_6 = true;
                break;
            case 7:
                ASSERT(saw_7 == false);
                saw_7 = true;
                break;
            case 8:
                ASSERT(saw_8 == false);
                saw_8 = true;
                break;
            case 9:
                ASSERT(saw_9 == false);
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

        EXPECT(s8s.find(0) == 0);
        EXPECT(s8s.find(1) == 1);
        EXPECT(s8s.find(2) == 2);
        EXPECT(s8s.find(3) == 3);
        EXPECT(s8s.find(4) == 4);
        EXPECT(s8s.find(5) == 5);
        EXPECT(s8s.find(6) == 6);
        EXPECT(s8s.find(7) == 7);
        EXPECT(s8s.find(8) == 8);
        EXPECT(s8s.find(9) == 9);
        EXPECT(!s8s.find(10).has_value());

        HashSet<s8> others;
        others.put_all(s8s);

        s8s.clear();
        EXPECT(s8s.count() == 0);
        EXPECT(others.count() == 10);
    }

    // Iteration should still work correctly if entry 0 is unoccupied.
    // We had a bug where its value would be returned even if it wasn't occupied.
    {
        HashSet<s32> set;
        set.put(0);

        bool seen = false;
        for (auto i : set) {
            EXPECT(i == 0);
            EXPECT(seen == false);
            seen = true;
        }
    }

    // Strings
    {
        HashSet<String> strings;
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

    // Pointers
    {
        HashSet<s32*> set;
        s32 a = 7;

        EXPECT(set.contains(&a) == false);
        set.put(&a);
        EXPECT(set.contains(&a));

        for (auto it : set) {
            EXPECT(*it == 7);
        }

        set.remove(&a);
        EXPECT(set.contains(&a) == false);

        for (auto it : set) {
            (void)it;
            EXPECT(false);
        }
    }
}
