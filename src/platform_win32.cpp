#pragma once

// platform_win32.cpp

#include <shellapi.h>

void openUrlUnsafe(char* url) {
	ShellExecute(null, "open", url, null, null, SW_SHOWNORMAL);
}