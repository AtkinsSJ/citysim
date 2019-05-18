#pragma once

struct String;

void log(SDL_LogPriority priority, char *format, std::initializer_list<String> args = {});

inline void logVerbose (char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_VERBOSE, format, args);}
inline void logDebug   (char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_DEBUG, format, args);}
inline void logInfo    (char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_INFO, format, args);}
inline void logWarn    (char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_WARN, format, args);}
inline void logError   (char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_ERROR, format, args);}
inline void logCritical(char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_CRITICAL, format, args);}


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