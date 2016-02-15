#pragma once

void *readFile(TemporaryMemoryArena *tempMemory, char *filename, char *mode)
{
	void *fileData = 0;

	SDL_RWops *file = SDL_RWFromFile(filename, mode);
	if (file)
	{
		size_t fileLength = (size_t) file->seek(file, 0, RW_SEEK_END);
		file->seek(file, 0, RW_SEEK_SET);

		fileData = allocate(tempMemory, fileLength + 1); // 1 longer to ensure a trailing null for strings!

		if (fileData)
		{
			file->read(file, fileData, fileLength, 1);
		}

		file->close(file);
	}
	
	return fileData;
}