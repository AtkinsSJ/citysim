#pragma once

// platform_linux.cpp
#include <ctime>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void openUrlUnsafe(char const* url)
{
	char const* format = "xdg-open '%s'";
	smm totalSize = strlen(format) + strlen(url);
	char* buffer = new char[totalSize];
	snprintf(buffer, sizeof(buffer), format, url);
	system(buffer);
	delete[] buffer;
}

u64 getCurrentUnixTimestamp()
{
	return time(null);
}

DateTime platform_getLocalTimeFromTimestamp(u64 unixTimestamp)
{
	DateTime result = {};
	result.unixTimestamp = unixTimestamp;

	time_t time = unixTimestamp;
	struct tm* timeInfo = localtime(&time);


	result.year = timeInfo->tm_year + 1900;
	result.month = (MonthOfYear)(timeInfo->tm_mon);
	result.dayOfMonth = timeInfo->tm_mday;
	result.hour = timeInfo->tm_hour;
	result.minute = timeInfo->tm_min;
	result.second = timeInfo->tm_sec;
	result.millisecond = 0;
	result.dayOfWeek = (DayOfWeek)((timeInfo->tm_wday + 6) % 7); // NB: Sunday is 0 in tm, but is 6 for us.


	return result;
}

String platform_constructPath(std::initializer_list<String> parts, bool appendWildcard)
{
	// @Copypasta: This is identical to the win32 version except with '/' instead of '\\'!
	StringBuilder stb = newStringBuilder(256);

	if (parts.size() > 0)
	{
		for (auto it = parts.begin(); it != parts.end(); it++)
		{
			if (it != parts.begin() && (stb.buffer[stb.length-1] != '/'))
			{
				append(&stb, '/');
			}
			append(&stb, *it);
			// Trim off a trailing null that might be there.
			if (stb.buffer[stb.length - 1] == '\0') stb.length--;
		}
	}

	if (appendWildcard)
	{
		append(&stb, "/*"_s);
	}

	append(&stb, '\0');

	String result = getString(&stb);

	ASSERT(isNullTerminated(result)); //Path strings must be null-terminated!

	return result;
}

bool platform_createDirectory(String _path)
{
	ASSERT(isNullTerminated(_path));

	if (mkdir(_path.chars, S_IRWXU) != 0)
	{
		int result = errno;
		if (result == EEXIST)
			return true;

		if (result == ENOENT)
		{
			// Part of the path doesn't exist, so we have to create it, piece by piece
			// We do a similar hack to the win32 version: A duplicate path, which we then swap each
			// `/` with a null byte and then back, to mkdir() one path segment at a time.
			String path = pushString(tempArena, _path);
			char *pos = path.chars;
			char *afterEndOfPath = path.chars + path.length;

			while (pos < afterEndOfPath)
			{
				// This double loop is actually intentional, it's just... weird.
				while (pos < afterEndOfPath)
				{
					if (*pos == '/')
					{
						*pos = '\0';
						break;
					}
					else
					{
						pos++;
					}
				}

				logInfo("Attempting to create directory: {0}"_s, {path});

				// Create the path
				if (mkdir(path.chars, S_IRWXU) != EEXIST)
				{
					logError("Unable to create directory `{0}` - failed to create `{1}`."_s, {_path, path});
					return false;
				}

				*pos = '/';
				pos++;
				continue;
			}


			return true;
		}
	}


	return false;
}

bool platform_deleteFile(String path)
{
	ASSERT(isNullTerminated(path));

	if (unlink(path.chars) == 0)
		return true;

	if (errno == ENOENT)
	{
		logInfo("Unable to delete file `{0}` - path does not exist"_s, {path});
	}

	return false;
}

struct FileInfo;

struct DirectoryListingHandle;
DirectoryListingHandle platform_beginDirectoryListing(String path, FileInfo *result)
{
	ASSERT(isNullTerminated(path));

	DirectoryListingHandle handle = {};

	// TODO: Implement this!

	return handle;
}

bool platform_nextFileInDirectory(DirectoryListingHandle *, FileInfo *)
{
	// TODO: Implement this!
	return false;
}

void platform_stopDirectoryListing(DirectoryListingHandle *)
{
	// TODO: Implement this!
}

struct DirectoryChangeWatchingHandle;
DirectoryChangeWatchingHandle platform_beginWatchingDirectory(String path)
{
	ASSERT(isNullTerminated(path));

	DirectoryChangeWatchingHandle handle = {};
	// TODO: Implement this!

	return handle;
}

bool platform_hasDirectoryChanged(DirectoryChangeWatchingHandle *handle)
{
	// TODO: Implement this!
	return false;
}

void platform_stopWatchingDirectory(DirectoryChangeWatchingHandle *handle)
{
	// TODO: Implement this!
}
