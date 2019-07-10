#pragma once

// platform_linux.cpp

void openUrlUnsafe(char* url) {
	char buffer[1024];
	ASSERT(strlen(url) < ArrayCount(buffer) - 16); //URL is too long!
	snprintf(buffer, sizeof(buffer), "xdg-open '%s'", url);
	system(buffer);
}