#pragma once

String readFileAsString(MemoryArena *memory, String filename)
{
	String result = {};

	SDL_RWops *file = SDL_RWFromFile(filename.chars, "r");
	if (file)
	{
		umm fileLength = (umm) file->seek(file, 0, RW_SEEK_END);
		file->seek(file, 0, RW_SEEK_SET);

 		// 1 longer to ensure a trailing null for strings!
		result = newString(memory, fileLength + 1);
		result.chars[fileLength] = 0;

		if (result.chars)
		{
			file->read(file, result.chars, fileLength, 1);
			result.length = fileLength;
		}

		file->close(file);

	}
	
	return result;
}

/*
 * @Deprecated This File struct can probably vanish now, because it's identical to a String.
 */

struct File
{
	umm length;
	u8 *data;
};

File readFile(MemoryArena *arena, char *filename)
{
	File result = {};

	SDL_RWops *file = SDL_RWFromFile(filename, "rb");
	if (file)
	{
		s64 length = file->seek(file, 0, RW_SEEK_END);
		file->seek(file, 0, RW_SEEK_SET);

		ASSERT(result.length <= s32Max, "File is too big to fit into an s32!");

		result.length = (umm) length;
		result.data = PushArray(arena, u8, result.length);

		if (result.data)
		{
			file->read(file, result.data, result.length, 1);
		}

		file->close(file);
	}
	
	return result;
}

struct LineReader
{
	String file;
	bool skipBlankLines;

	int32 pos;
	int32 lineNumber;

	bool removeComments;
	char commentChar;
};

LineReader startFile(String file, bool skipBlankLines=true, bool removeComments=true, char commentChar = '#')
{
	LineReader result = {};
	result.file = file;
	result.pos = 0;
	result.lineNumber = 0;
	result.removeComments = removeComments;
	result.commentChar = commentChar;
	result.skipBlankLines = skipBlankLines;

	return result;
}

String nextLine(LineReader *reader)
{
	String line = {};

	do
	{
		// get next line
		++reader->lineNumber;
		line.chars = reader->file.chars + reader->pos;
		line.length = 0;
		while (!isNewline(reader->file.chars[reader->pos]) && (reader->pos < reader->file.length))
		{
			++reader->pos;
			++line.length;
		}
		++reader->pos;
		// fix for windows' stupid double-newline.
		if (isNewline(reader->file.chars[reader->pos]) && (reader->file.chars[reader->pos] != reader->file.chars[reader->pos-1]))
		{
			++reader->pos;
		}

		// trim the comment
		if (reader->removeComments)
		{
			for (int32 p=0; p<line.length; p++)
			{
				if (line.chars[p] == reader->commentChar)
				{
					line.length = p;
					break;
				}
			}
		}

		// trim front whitespace
		line = trimStart(line);
		// trim back whitespace
		line = trimEnd(line);

		if (!reader->skipBlankLines) break;
	}
	while ((line.length <= 0));

	// consoleWriteLine(myprintf("LINE {1}: \"{0}\"", {line, formatInt(reader->lineNumber)}));

	return line;
}