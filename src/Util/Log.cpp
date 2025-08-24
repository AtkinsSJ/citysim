/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Log.h"
#include <Util/String.h>

void log(SDL_LogPriority priority, String format, std::initializer_list<String> args)
{
    String text = myprintf(format, args, true);
    SDL_LogMessage(SDL_LOG_CATEGORY_CUSTOM, priority, "%s", text.chars);
}

void logVerbose(String format, std::initializer_list<String> args)
{
    log(SDL_LOG_PRIORITY_VERBOSE, format, args);
}
void logDebug(String format, std::initializer_list<String> args)
{
    log(SDL_LOG_PRIORITY_DEBUG, format, args);
}
void logInfo(String format, std::initializer_list<String> args)
{
    log(SDL_LOG_PRIORITY_INFO, format, args);
}
void logWarn(String format, std::initializer_list<String> args)
{
    log(SDL_LOG_PRIORITY_WARN, format, args);
}
void logError(String format, std::initializer_list<String> args)
{
    log(SDL_LOG_PRIORITY_ERROR, format, args);
    DEBUG_BREAK();
}
void logCritical(String format, std::initializer_list<String> args)
{
    log(SDL_LOG_PRIORITY_CRITICAL, format, args);
    ASSERT(!"Critical error");
}
