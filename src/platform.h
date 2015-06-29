#pragma once

// platform.h

// Open the given URL in the user's default browser.
// NB: No check is done to ensure the URL is safe to use in a console command!
// Only use this for URLs we know for sure are OK.
void openUrlUnsafe(char* url);

#ifdef __linux__
#include "platform_linux.cpp"
#else // Windows
#include "platform_win32.cpp"
#endif