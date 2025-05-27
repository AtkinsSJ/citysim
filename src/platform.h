#pragma once

// platform.h
#ifdef __linux__
// stuff
#else // Windows
#    define NOMINMAX
#    include <shellapi.h>
#    include <windows.h>
#endif

// Open the given URL in the user's default browser.
// NB: No check is done to ensure the URL is safe to use in a console command!
// Only use this for URLs we know for sure are OK.
void openUrlUnsafe(char const* url);

u64 getCurrentUnixTimestamp();
DateTime platform_getLocalTimeFromTimestamp(u64 unixTimestamp);

String platform_constructPath(std::initializer_list<String> parts, bool appendWildcard);

bool platform_createDirectory(String path);
bool platform_deleteFile(String path);

struct FileInfo;

struct DirectoryListingHandle;
DirectoryListingHandle platform_beginDirectoryListing(String path, FileInfo* result);
bool platform_nextFileInDirectory(DirectoryListingHandle* handle, FileInfo* result);
void platform_stopDirectoryListing(DirectoryListingHandle* handle);

struct DirectoryChangeWatchingHandle;
DirectoryChangeWatchingHandle platform_beginWatchingDirectory(String path);
bool platform_hasDirectoryChanged(DirectoryChangeWatchingHandle* handle);
void platform_stopWatchingDirectory(DirectoryChangeWatchingHandle* handle);
