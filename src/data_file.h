#pragma once

struct LineReader_Old
{
	String filename;
	Blob data;

	bool skipBlankLines;

	smm pos;
	smm lineNumber;

	bool removeComments;
	char commentChar;

	bool hasNextLine;
	String nextLine;
	smm nextLineNumber;
};

enum LineReaderFlags
{
	LineReader_SkipBlankLines         = 1 << 0,
	LineReader_RemoveTrailingComments = 1 << 1,
	DefaultLineReaderFlags = LineReader_SkipBlankLines | LineReader_RemoveTrailingComments,
};

LineReader_Old readLines_old(String filename, Blob data, u32 flags = DefaultLineReaderFlags, char commentChar = '#');

String nextLine(LineReader_Old *reader);
bool isDone(LineReader_Old *reader);

void warn(LineReader_Old *reader, char *message, std::initializer_list<String> args = {});
void error(LineReader_Old *reader, char *message, std::initializer_list<String> args = {});

Maybe<s64> readInt(LineReader_Old *reader, String command, String arguments);
Maybe<bool> readBool(LineReader_Old *reader, String command, String arguments);
Maybe<V4> readColor(LineReader_Old *reader, String command, String arguments);
Maybe<u32> readAlignment(LineReader_Old *reader, String command, String arguments);
Maybe<String> readTextureDefinition(LineReader_Old *reader, String tokens);
Maybe<struct EffectRadius> readEffectRadius(LineReader_Old *reader, String command, String tokens);

//
// Internal
//
void readNextLineInternal(LineReader_Old *reader);
