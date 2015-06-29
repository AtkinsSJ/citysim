#pragma once

// platform_win32.cpp

#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>

void openUrlUnsafe(char* url) {
	ShellExecute(null, "open", url, null, null, SW_SHOWNORMAL);
}