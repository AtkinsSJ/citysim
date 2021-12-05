#pragma once

void log(SDL_LogPriority priority, String format, std::initializer_list<String> args)
{
	String text = myprintf(format, args, true);
	SDL_LogMessage(SDL_LOG_CATEGORY_CUSTOM, priority, "%s", text.chars);
}

inline void logVerbose (String format, std::initializer_list<String> args)
{
	log(SDL_LOG_PRIORITY_VERBOSE, format, args);
}
inline void logDebug   (String format, std::initializer_list<String> args)
{
	log(SDL_LOG_PRIORITY_DEBUG, format, args);
}
inline void logInfo    (String format, std::initializer_list<String> args)
{
	log(SDL_LOG_PRIORITY_INFO, format, args);
}
inline void logWarn    (String format, std::initializer_list<String> args)
{
	log(SDL_LOG_PRIORITY_WARN, format, args);
}
inline void logError   (String format, std::initializer_list<String> args)
{
	log(SDL_LOG_PRIORITY_ERROR, format, args);
	DEBUG_BREAK();
}
inline void logCritical(String format, std::initializer_list<String> args)
{
	log(SDL_LOG_PRIORITY_CRITICAL, format, args);
	ASSERT(!"Critical error");
}

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

		default:
			style = CLS_Default;
			break;
	}

	consoleWriteLine(makeString(message), style);
}

void enableCustomLogger()
{
	SDL_LogGetOutputFunction(&defaultLogger, &defaultLoggerUserData);
	SDL_LogSetOutputFunction(&customLogOutputFunction, 0);
}