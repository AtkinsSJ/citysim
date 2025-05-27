#pragma once

struct String;

void log(SDL_LogPriority priority, String format, std::initializer_list<String> args = {});

void logVerbose(String format, std::initializer_list<String> args = {});
void logDebug(String format, std::initializer_list<String> args = {});
void logInfo(String format, std::initializer_list<String> args = {});
void logWarn(String format, std::initializer_list<String> args = {});
void logError(String format, std::initializer_list<String> args = {});
void logCritical(String format, std::initializer_list<String> args = {});

void enableCustomLogger();
