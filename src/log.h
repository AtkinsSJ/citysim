#pragma once

struct String;

void log(char *format, std::initializer_list<String> args = {});

#define LOGFUNCDEF(TYPE) void log##TYPE(char *format, std::initializer_list<String> args = {});

LOGFUNCDEF(Verbose);
LOGFUNCDEF(Debug);
LOGFUNCDEF(Info);
LOGFUNCDEF(Warn);
LOGFUNCDEF(Error);
LOGFUNCDEF(Critical);


#if BUILD_DEBUG
	#if defined(_MSC_VER)
		// Pauses the MS debugger
		#define DEBUG_BREAK() __debugbreak()
	#else
		#define DEBUG_BREAK() {*(int *)0 = 0;}
	#endif

	void ASSERT(bool expr, char *format, std::initializer_list<String> args = {})
	{
		if(!(expr))
		{
			logError(format, args);
			DEBUG_BREAK();
		}
	}
#else
	// We put a dummy thing here to stop the compiler complaining
	// "local variable is initialized but not referenced"
	// if a variable is only used in expr.
	#define ASSERT(expr, ...) /*nothing*/ if (expr) {};
#endif