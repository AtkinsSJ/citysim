#pragma once

enum ConsoleLineStyleID
{
	CLS_Default,
	CLS_InputEcho,
	CLS_Success,
	CLS_Error,

	CLS_Input,

	CLS_COUNT
};

struct ConsoleLineStyle
{
	V4 textColor;
};

struct ConsoleOutputLine
{
	String text;
	ConsoleLineStyleID style;
};

struct Console
{
	struct BitmapFont *font;
	ConsoleLineStyle styles[CLS_COUNT];

	real32 currentHeight;
	real32 targetHeight;
	real32 expandedHeight; // config!
	real32 openSpeed; // % per second

	TextInput input;
	int32 charWidth;

	int32 outputLineCount;
	ConsoleOutputLine *outputLines;
	int32 currentOutputLine;
	int32 scrollPos; // first line to draw, just above the console input
};
Console *globalConsole;

const int32 consoleLineLength = 255;

String *consoleNextOutputLine(Console *console, ConsoleLineStyleID style=CLS_Default)
{
	ConsoleOutputLine *line = console->outputLines + console->currentOutputLine;
	line->style = style;

	String *result = &line->text;
	console->currentOutputLine = WRAP(console->currentOutputLine + 1, console->outputLineCount);

	return result;
}

void consoleWriteLine(String text, ConsoleLineStyleID style=CLS_Default);
inline void consoleWriteLine(char *text, ConsoleLineStyleID style=CLS_Default)
{
	consoleWriteLine(stringFromChars(text), style);
}

inline int32 consoleMaxScrollPos(Console *console)
{
	int32 result = MIN(console->currentOutputLine-1, console->outputLineCount)-1;
	return result;
}