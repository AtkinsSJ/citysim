#pragma once

void log(char *format, std::initializer_list<String> args)
{
	String text = myprintf(stringFromChars(format), args, true);
	SDL_Log("%s", text);
}

#define LOGFUNC(TYPE) void log##TYPE(char *format, std::initializer_list<String> args) \
{ \
	String text = myprintf(stringFromChars(format), args, true); \
	SDL_Log##TYPE(SDL_LOG_CATEGORY_CUSTOM, "%s", text); \
}

LOGFUNC(Verbose);
LOGFUNC(Debug);
LOGFUNC(Info);
LOGFUNC(Warn);
LOGFUNC(Error);
LOGFUNC(Critical);

/*
 * Our custom logger posts the message into the built-in console, AND then sends it off to SDL's
 * usual logger. This way it's still useful even if our console is broken. (Which will happen!)
 */

static SDL_LogOutputFunction defaultLogger;
static void *defaultLoggerUserData;

void customLogOutputFunction(void *userdata, int category, SDL_LogPriority priority, const char *message)
{
	defaultLogger(defaultLoggerUserData, category, priority, message);
	consoleWriteLine((char*)message, CLS_Default);
}

void enableCustomLogger()
{
	SDL_LogGetOutputFunction(&defaultLogger, &defaultLoggerUserData);
	SDL_LogSetOutputFunction(&customLogOutputFunction, 0);
}