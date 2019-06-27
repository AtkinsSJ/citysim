#pragma once

struct String;

void log(SDL_LogPriority priority, char *format, std::initializer_list<String> args = {});

inline void logVerbose (char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_VERBOSE, format, args);}
inline void logDebug   (char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_DEBUG, format, args);}
inline void logInfo    (char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_INFO, format, args);}
inline void logWarn    (char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_WARN, format, args);}
inline void logError   (char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_ERROR, format, args);}
inline void logCritical(char *format, std::initializer_list<String> args = {}) {log(SDL_LOG_PRIORITY_CRITICAL, format, args);}

void enableCustomLogger();