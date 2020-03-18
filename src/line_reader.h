#pragma once

struct LineReaderPosition
{
	String currentLine;
	smm currentLineNumber;
	String lineRemainder;
	smm startOfNextLine;
};

struct LineReader
{
	String filename;
	Blob data;

	LineReaderPosition position;

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
LineReaderPosition savePosition(LineReader *reader);
void restorePosition(LineReader *reader, LineReaderPosition position);
void restart(LineReader *reader);

// How many lines are in the file?
s32 countLines(Blob data);

bool loadNextLine(LineReader *reader);
String getLine(LineReader *reader);
String getRemainderOfLine(LineReader *reader);

// How many times does the given property appear in the current :Command block?
s32 countPropertyOccurrences(LineReader *reader, String propertyName);

String readToken(LineReader *reader, char splitChar=0);
String peekToken(LineReader *reader, char splitChar=0);

template<typename T>
Maybe<T>    readInt  (LineReader *reader, bool isOptional=false, char splitChar=0);
Maybe<f64>  readFloat(LineReader *reader, bool isOptional=false, char splitChar=0);
Maybe<bool> readBool (LineReader *reader, bool isOptional=false, char splitChar=0);

Maybe<V4> readColor(LineReader *reader);
Maybe<u32> readAlignment(LineReader *reader);
Maybe<struct EffectRadius> readEffectRadius(LineReader *reader);
Maybe<V2I> readV2I(LineReader *reader);

void warn(LineReader *reader, String message, std::initializer_list<String> args = {});
void error(LineReader *reader, String message, std::initializer_list<String> args = {});
