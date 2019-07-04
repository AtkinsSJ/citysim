#pragma once

struct String;

void log(SDL_LogPriority priority, char *format, std::initializer_list<String> args = {});

void logVerbose (char *format, std::initializer_list<String> args = {});
void logDebug   (char *format, std::initializer_list<String> args = {});
void logInfo    (char *format, std::initializer_list<String> args = {});
void logWarn    (char *format, std::initializer_list<String> args = {});
void logError   (char *format, std::initializer_list<String> args = {});
void logCritical(char *format, std::initializer_list<String> args = {});

void enableCustomLogger();