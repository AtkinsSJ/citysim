/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <SDL2/SDL_log.h>
#include <Util/Forward.h>
#include <initializer_list>

void log(SDL_LogPriority priority, String format, std::initializer_list<String> args = {});

void logVerbose(String format, std::initializer_list<String> args = {});
void logDebug(String format, std::initializer_list<String> args = {});
void logInfo(String format, std::initializer_list<String> args = {});
void logWarn(String format, std::initializer_list<String> args = {});
void logError(String format, std::initializer_list<String> args = {});
void logCritical(String format, std::initializer_list<String> args = {});

void enableCustomLogger();
