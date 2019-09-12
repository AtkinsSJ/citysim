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

LineReader readLines(String filename, Blob data, u32 flags = DefaultLineReaderFlags, char commentChar = '#');

bool loadNextLine(LineReader *reader);
String getLine(LineReader *reader);
String getRemainderOfLine(LineReader *reader);

String readToken(LineReader *reader);
Maybe<s64> readInt(LineReader *reader);
Maybe<bool> readBool(LineReader *reader);
Maybe<V4> readColor(LineReader *reader);
Maybe<u32> readAlignment(LineReader *reader);
Maybe<String> readTextureDefinition(LineReader *reader);
Maybe<struct EffectRadius> readEffectRadius(LineReader *reader);

void warn(LineReader *reader, char *message, std::initializer_list<String> args = {});
void error(LineReader *reader, char *message, std::initializer_list<String> args = {});
