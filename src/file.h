#pragma once

struct File
{
	String name;
	bool isLoaded;
	
	smm length;
	u8* data;
};

File readFile(MemoryArena *memory, String filename)
{
	File result = {};
	result.name = filename;
	result.isLoaded = false;

	SDL_RWops *file = SDL_RWFromFile(filename.chars, "rb");
	if (file)
	{
		smm fileLength = (smm) file->seek(file, 0, RW_SEEK_END);
		file->seek(file, 0, RW_SEEK_SET);

		result.data = PushArray(memory, u8, fileLength);

		if (result.data)
		{
			file->read(file, result.data, fileLength, 1);
			result.length = fileLength;
			result.isLoaded = true;
		}

		file->close(file);

	}
	
	return result;
}

String readFileAsString(MemoryArena *memory, String filename)
{
	File file = readFile(memory, filename);
	String result = makeString((char*)file.data, file.length);

	return result;
}

struct LineReader
{
	File file;
	bool skipBlankLines;

	smm pos;
	smm lineNumber;

	bool removeComments;
	char commentChar;
};

LineReader startFile(File file, bool skipBlankLines=true, bool removeComments=true, char commentChar = '#')
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

void warn(LineReader *reader, char *message, std::initializer_list<String> args = {})
{
	String text = myprintf(message, args, false);
	logWarn("{0}:{1} - {2}", {reader->file.name, formatInt(reader->lineNumber), text});
}

void error(LineReader *reader, char *message, std::initializer_list<String> args = {})
{
	String text = myprintf(message, args, false);
	logError("{0}:{1} - {2}", {reader->file.name, formatInt(reader->lineNumber), text});
}

String nextLine(LineReader *reader)
{
	String line = {};

	do
	{
		// get next line
		++reader->lineNumber;
		line.chars = (char *)(reader->file.data + reader->pos);
		line.length = 0;
		while (!isNewline(reader->file.data[reader->pos]) && (reader->pos < reader->file.length))
		{
			++reader->pos;
			++line.length;
		}
		++reader->pos;
		// fix for windows' stupid double-newline.
		if (isNewline(reader->file.data[reader->pos]) && (reader->file.data[reader->pos] != reader->file.data[reader->pos-1]))
		{
			++reader->pos;
		}

		// trim the comment
		if (reader->removeComments)
		{
			for (s32 p=0; p<line.length; p++)
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

	return line;
}
