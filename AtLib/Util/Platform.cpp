/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Platform.h"
#include <Util/String.h>
#include <Util/StringView.h>

#if OS_LINUX
#    include <cstdlib>
#elif OS_WINDOWS
#    include <shellapi.h>
#endif

void open_url_unsafe(StringView url)
{
#if OS_LINUX
    auto command = myprintf("xdg-open '{}'"_s, { url }, true);
    system(command.raw_pointer_to_characters());
#elif OS_WINDOWS
    auto null_terminated_url = myprintf("{}"_s, { url }, true);
    ShellExecute(nullptr, "open", null_terminated_url.raw_pointer_to_characters(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
}
