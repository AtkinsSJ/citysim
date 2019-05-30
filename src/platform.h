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

String platform_constructPath(std::initializer_list<String> parts, bool appendWildcard);

struct FileInfo;
struct DirectoryListingHandle;
DirectoryListingHandle platform_beginDirectoryListing(String path, FileInfo *result);
bool platform_nextFileInDirectory(DirectoryListingHandle *handle, FileInfo *result);

struct DirectoryChangeWatchingHandle;
DirectoryChangeWatchingHandle platform_beginWatchingDirectory(String path);
bool platform_hasDirectoryChanged(DirectoryChangeWatchingHandle *handle);
void platform_stopWatchingDirectory(DirectoryChangeWatchingHandle *handle);
