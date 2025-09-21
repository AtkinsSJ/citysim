/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#if defined(__linux__)
#    define OS_LINUX 1
#elif defined(_WIN32)
#    define OS_WINDOWS 1
#endif
