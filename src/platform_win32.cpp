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

	handle.windows.hFile = FindFirstFile(path.chars, &findFileData);

	if (handle.windows.hFile == INVALID_HANDLE_VALUE)
	{
		handle.isValid = false;
		handle.errorCode = (u32)GetLastError();
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