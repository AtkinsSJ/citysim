/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <SDL2/SDL_assert.h>

#if BUILD_DEBUG
#    define DEBUG_BREAK() SDL_TriggerBreakpoint()
#else
#    define DEBUG_BREAK()
#endif

// SDL already does work to avoid MSVC warnings, but we still get them!
// So, I've added what SHOULD be a no-op wrapper around it, using the (a,b) comma trick.
// (Well, it's not a no-op in debug builds, but the release optimiser does make it a no-op!)
// - Sam, 10/07/2019
#define ASSERT(expr) SDL_assert(expr)
#define ASSERT_PARANOID(expr) SDL_assert_paranoid(expr)
#define ASSERT_RELEASE(expr) SDL_assert_release(expr)

#define INVALID_DEFAULT_CASE \
    default:                 \
        ASSERT(false);       \
        break;

#define VERIFY_NOT_REACHED() \
    ASSERT_RELEASE(false);   \
    __builtin_unreachable();
