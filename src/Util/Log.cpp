/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Log.h"
#include "console.h"
#include <UI/UITheme.h>
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

/*
 * Our custom logger posts the message into the built-in console, AND then sends it off to SDL's
 * usual logger. This way it's still useful even if our console is broken. (Which will happen!)
 */

static SDL_LogOutputFunction defaultLogger;
static void* defaultLoggerUserData;

void customLogOutputFunction(void* /*userdata*/, int category, SDL_LogPriority priority, char const* message)
{
    defaultLogger(defaultLoggerUserData, category, priority, message);

    ConsoleLineStyle style = ConsoleLineStyle::Default;

    switch (priority) {
    case SDL_LOG_PRIORITY_WARN:
        style = ConsoleLineStyle::Warning;
        break;

    case SDL_LOG_PRIORITY_ERROR:
    case SDL_LOG_PRIORITY_CRITICAL:
        style = ConsoleLineStyle::Error;
        break;

    default:
        style = ConsoleLineStyle::Default;
        break;
    }

    consoleWriteLine(makeString(message), style);
}

void enableCustomLogger()
{
    SDL_LogGetOutputFunction(&defaultLogger, &defaultLoggerUserData);
    SDL_LogSetOutputFunction(&customLogOutputFunction, 0);
}
