/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>

#define EXPECT(Condition)                                                               \
    g_test_context.current_line = __LINE__;                                             \
    if (!(Condition)) {                                                                 \
        printf("\x1b[31m❎ FAIL %s:%d: `%s`\x1b[0m\n", __FILE__, __LINE__, #Condition); \
        g_test_context.failures++;                                                      \
    } else {                                                                            \
        printf("\x1b[32m✅ PASS %s:%d: `%s`\x1b[0m\n", __FILE__, __LINE__, #Condition); \
    }

struct TestContext {
    int failures { 0 };
    int current_line { 0 };
};

inline TestContext g_test_context {};

void test_main();

int main(int, char*[])
{
    SDL_SetAssertionHandler([](SDL_AssertData const* assert_data, void*) {
        g_test_context.failures++;
        printf("\x1b[31m💥 ASSERT failed at %s:%d: `%s`\x1b[0m\n", assert_data->filename, assert_data->linenum, assert_data->condition);
        printf("... Most recent EXPECT was line %d of %s\n", g_test_context.current_line, __FILE__);
        return SDL_ASSERTION_IGNORE;
    },
        nullptr);

    test_main();
    return g_test_context.failures;
}
