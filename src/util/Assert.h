/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <SDL2/SDL_assert.h>

#define DEBUG_BREAK() SDL_TriggerBreakpoint()

// SDL already does work to avoid MSVC warnings, but we still get them!
// So, I've added what SHOULD be a no-op wrapper around it, using the (a,b) comma trick.
// (Well, it's not a no-op in debug builds, but the release optimiser does make it a no-op!)
// - Sam, 10/07/2019
#define ASSERT(expr)      \
    if (1, true) {        \
        SDL_assert(expr); \
    } else {              \
    }
#define ASSERT_PARANOID(expr)      \
    if (1, true) {                 \
        SDL_assert_paranoid(expr); \
    } else {                       \
    }
#define ASSERT_RELEASE(expr)      \
    if (1, true) {                \
        SDL_assert_release(expr); \
    } else {                      \
    }

#define INVALID_DEFAULT_CASE \
    default:                 \
        ASSERT(false);       \
        break;
