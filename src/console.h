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
	StringBuffer buffer;
	ConsoleLineStyleID style;
};

struct Console
{
	bool isVisible;
	struct BitmapFont *font;
	ConsoleLineStyle styles[CLS_COUNT];
	real32 height;

	StringBuffer input;
	int32 caretPos;
	int32 charWidth;

	int32 outputLineCount;
	ConsoleOutputLine *outputLines;
	int32 currentOutputLine;

	real32 caretFlashCounter;
};
Console *globalConsole;

const int32 consoleLineLength = 255;

StringBuffer *consoleNextOutputLine(Console *console, ConsoleLineStyleID style=CLS_Default)
{
	ConsoleOutputLine *line = console->outputLines + console->currentOutputLine;
	line->style = style;

	StringBuffer *result = &line->buffer;
	console->currentOutputLine = WRAP(console->currentOutputLine + 1, console->outputLineCount);

	clear(result);

	return result;
}

void consoleWriteLine(char *text, ConsoleLineStyleID style=CLS_Default);