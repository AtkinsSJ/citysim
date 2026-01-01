/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Harness/Harness.h"
#include <Util/Function.h>

void test_main()
{
    // Just a capture
    {
        int counter = 0;
        Function<void()> empty_function = [&]() {
            counter++;
        };
        EXPECT(counter == 0);
        empty_function();
        EXPECT(counter == 1);
        empty_function();
        EXPECT(counter == 2);
    }

    // An argument and a capture
    {
        int counter = 0;
        Function<void(int)> increase = [&counter](int increment) {
            counter += increment;
        };
        EXPECT(counter == 0);
        increase(1);
        EXPECT(counter == 1);
        increase(10);
        EXPECT(counter == 11);
    }

    // Multiple arguments, and a return value
    {
        Function<int(int, int)> multiply = [](int first, int second) {
            return first * second;
        };
        EXPECT(multiply(1, 1) == 1);
        EXPECT(multiply(10, 5) == 50);
        EXPECT(multiply(6, 7) == 42);
    }

    // Reassignment
    {
        Function<int(int)> do_something;
        EXPECT(!do_something);
        do_something = [](int a) { return a; };
        EXPECT(do_something);
        EXPECT(do_something(1) == 1);
        EXPECT(do_something(5) == 5);
        EXPECT(do_something(100) == 100);

        do_something = [](int a) { return -a; };
        EXPECT(do_something);
        EXPECT(do_something(1) == -1);
        EXPECT(do_something(5) == -5);
        EXPECT(do_something(100) == -100);

        auto do_it = move(do_something);
        EXPECT(do_it);
        EXPECT(!do_something);
        EXPECT(do_it(100) == -100);
    }
}
