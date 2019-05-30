#pragma once

// platform_win32.cpp

void openUrlUnsafe(char* url) {
	ShellExecute(null, "open", url, null, null, SW_SHOWNORMAL);
}

String platform_constructPath(std::initializer_list<String> parts, bool appendWildcard)
{
	StringBuilder stb = newStringBuilder(256);

	if (parts.size() > 0)
	{
		for (auto it = parts.begin(); it != parts.end(); it++)
		{
			if (it != parts.begin()) append(&stb, '\\');
			append(&stb, *it);
			// Trim off a trailing null that might be there.
			if (stb.buffer[stb.length - 1] == '\0') stb.length--;
		}
	}

	if (appendWildcard)
	{
		append(&stb, "\\*");
	}

	append(&stb, '\0');

	String result = getString(&stb);

	ASSERT(isNullTerminated(result), "Path strings must be null-terminated!");

	return result;
}

inline void fillFileInfo(WIN32_FIND_DATA *findFileData, FileInfo *result)
{
	result->filename = pushString(globalFrameTempArena, findFileData->cFileName);
	u64 fileSize = ((u64)findFileData->nFileSizeHigh << 32) + findFileData->nFileSizeLow;
	result->size = (smm)fileSize; // NB: Theoretically it could be more than s64Max, but that seems unlikely?

	result->flags = 0;
	if (findFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)  result->flags |= FileFlag_Directory;
	if (findFileData->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)     result->flags |= FileFlag_Hidden;
	if (findFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)   result->flags |= FileFlag_ReadOnly;
}

DirectoryListingHandle platform_beginDirectoryListing(String path, FileInfo *result)
{
	DirectoryListingHandle handle = {};
	WIN32_FIND_DATA findFileData;

	handle.path = path;

	handle.windows.hFile = FindFirstFile(path.chars, &findFileData);

	if (handle.windows.hFile == INVALID_HANDLE_VALUE)
	{
		handle.isValid = false;
		u32 errorCode = (u32)GetLastError();
		handle.errorCode = errorCode;
		logError("Failed to read directory listing in \"{0}\". (Error {1})", {handle.path, formatInt(errorCode)});
	}
	else
	{
		handle.isValid = true;
		fillFileInfo(&findFileData, result);
	}

	return handle;
}

bool platform_nextFileInDirectory(DirectoryListingHandle *handle, FileInfo *result)
{
	WIN32_FIND_DATA findFileData;
	bool succeeded = FindNextFile(handle->windows.hFile, &findFileData) != 0;

	if (succeeded)
	{
		fillFileInfo(&findFileData, result);
	}
	else
	{
		FindClose(handle->windows.hFile);
	}

	return succeeded;
}

DirectoryChangeWatchingHandle platform_beginWatchingDirectory(String path)
{
	DirectoryChangeWatchingHandle handle = {};
	handle.path = path;

	handle.windows.handle = FindFirstChangeNotification(path.chars, true, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);

	if (handle.windows.handle == INVALID_HANDLE_VALUE)
	{
		handle.isValid = false;
		u32 errorCode = (u32)GetLastError();
		handle.errorCode = errorCode;
		logError("Failed to set notification for file changes in \"{0}\". (Error {1})", {handle.path, formatInt(errorCode)});
	}
	else
	{
		handle.isValid = true;
	}

	return handle;
}

bool platform_hasDirectoryChanged(DirectoryChangeWatchingHandle *handle)
{
	bool filesChanged = false;

	DWORD waitResult = WaitForSingleObject(handle->windows.handle, 0);
	switch (waitResult)
	{
		case WAIT_FAILED: 
		{
			// Something broke
			handle->isValid = false;
			u32 errorCode = (u32)GetLastError();
			handle->errorCode = errorCode;
			logError("Failed to poll for file changes in \"{0}\". (Error {1})", {handle->path, formatInt(errorCode)});
		} break;

		case WAIT_TIMEOUT: 
		{
			// Nothing to report
			filesChanged = false;
		} break;

		case WAIT_ABANDONED: 
		{
			// Something mutex-related, I think we can ignore this?
			// https://docs.microsoft.com/en-gb/windows/desktop/api/synchapi/nf-synchapi-waitforsingleobject
		} break;

		case WAIT_OBJECT_0: 
		{
			// We got a result!
			filesChanged = true;

			// Re-set the notification
			if (FindNextChangeNotification(handle->windows.handle) == false)
			{
				// something broke
				handle->isValid = false;
				u32 errorCode = (u32)GetLastError();
				handle->errorCode = errorCode;
				logError("Failed to re-set notification for file changes in \"{0}\". (Error {1})", {handle->path, formatInt(errorCode)});
			}
		} break;
	}

	return filesChanged;
}

void platform_stopWatchingDirectory(DirectoryChangeWatchingHandle *handle)
{
	FindCloseChangeNotification(handle->windows.handle);
	handle->windows.handle = INVALID_HANDLE_VALUE;
}
