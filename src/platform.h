#pragma once

// platform.h
#ifdef __linux__
// stuff
#else // Windows
#include <windows.h>
#include <shellapi.h>
#endif

// Open the given URL in the user's default browser.
// NB: No check is done to ensure the URL is safe to use in a console command!
// Only use this for URLs we know for sure are OK.
void openUrlUnsafe(char* url);


struct DirectoryListingHandle;
struct FileInfo;

String platform_constructPath(std::initializer_list<String> parts, bool appendWildcard);
DirectoryListingHandle platform_beginDirectoryListing(String path, FileInfo *result);
bool platform_nextFileInDirectory(DirectoryListingHandle *handle, FileInfo *result);
