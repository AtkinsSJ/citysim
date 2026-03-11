/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#ifdef __linux__
#    define OS_LINUX 1
#elifdef _WIN32
#    define OS_WINDOWS 1
#endif

#ifndef OS_LINUX
#    define OS_LINUX 0
#endif

#ifndef OS_WINDOWS
#    define OS_WINDOWS 0
#endif

#include <Util/Forward.h>

void open_url_unsafe(StringView url);
