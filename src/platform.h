/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Forward.h>
#include <Util/Time.h>

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
