#pragma once

void log(char *format, std::initializer_list<String> args)
{
	String text = myprintf(makeString(format), args, true);
	SDL_Log("%s", text);
}

#define LOGFUNC(TYPE) void log##TYPE(char *format, std::initializer_list<String> args) \
{ \
	String text = myprintf(makeString(format), args, true); \
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
	userdata = userdata; // To shut up the "unreferenced formal parameter" compiler warning.
	 
	defaultLogger(defaultLoggerUserData, category, priority, message);

	ConsoleLineStyleID style = CLS_Default;

	switch (priority)
	{
		case SDL_LOG_PRIORITY_WARN:
			style = CLS_Warning;
			break;

		case SDL_LOG_PRIORITY_ERROR:
		case SDL_LOG_PRIORITY_CRITICAL:
			style = CLS_Error;
			break;
	}

	consoleWriteLine((char*)message, style);
}

void enableCustomLogger()
{
	SDL_LogGetOutputFunction(&defaultLogger, &defaultLoggerUserData);
	SDL_LogSetOutputFunction(&customLogOutputFunction, 0);
}