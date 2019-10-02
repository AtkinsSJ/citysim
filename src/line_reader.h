#pragma once

struct LineReader
{
	String filename;
	Blob data;

	String currentLine;
	smm currentLineNumber;
	String lineRemainder;

	smm startOfNextLine;

	bool skipBlankLines;
	bool removeComments;
	char commentChar;
};

enum LineReaderFlags
{
	LineReader_SkipBlankLines         = 1 << 0,
	LineReader_RemoveTrailingComments = 1 << 1,
	DefaultLineReaderFlags = LineReader_SkipBlankLines | LineReader_RemoveTrailingComments,
};

LineReader readLines(String filename, Blob data, u32 flags = DefaultLineReaderFlags, char commentChar = '#');

s32 countLines(Blob data);

bool loadNextLine(LineReader *reader);
String getLine(LineReader *reader);
String getRemainderOfLine(LineReader *reader);

String readToken(LineReader *reader, char splitChar=0);
Maybe<s64> readInt(LineReader *reader);
Maybe<f64> readFloat(LineReader *reader);
Maybe<bool> readBool(LineReader *reader);
Maybe<V4> readColor(LineReader *reader);
Maybe<u32> readAlignment(LineReader *reader);
Maybe<String> readTextureDefinition(LineReader *reader);
Maybe<struct EffectRadius> readEffectRadius(LineReader *reader);

void warn(LineReader *reader, String message, std::initializer_list<String> args = {});
void error(LineReader *reader, String message, std::initializer_list<String> args = {});
