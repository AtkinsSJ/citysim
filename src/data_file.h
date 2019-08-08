#pragma once

struct LineReader
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

LineReader readLines(String filename, Blob data, bool skipBlankLines=true, bool removeComments=true, char commentChar = '#');

String nextLine(LineReader *reader);
bool isDone(LineReader *reader);

void warn(LineReader *reader, char *message, std::initializer_list<String> args = {});
void error(LineReader *reader, char *message, std::initializer_list<String> args = {});

s64 readInt(LineReader *reader, String command, String arguments);
bool readBool(LineReader *reader, String command, String arguments);
V4 readColor255(LineReader *reader, String command, String arguments);
u32 readAlignment(LineReader *reader, String command, String arguments);
String readTextureDefinition(LineReader *reader, String tokens);

//
// Internal
//
void readNextLineInternal(LineReader *reader);
