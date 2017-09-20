#pragma once

void log(char *format, std::initializer_list<String> args = {})
{
	String text = myprintf(stringFromChars(format), args, true);
	SDL_Log("%s", text);
}

#define LOGFUNC(TYPE) void log##TYPE(char *format, std::initializer_list<String> args = {}) \
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