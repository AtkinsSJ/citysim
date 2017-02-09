#pragma once

enum ConsoleLineStyleID
{
	CLS_Default,
	CLS_Input,
	CLS_InputEcho,
	CLS_Error,

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
	int32 outputLineCount;
	ConsoleOutputLine *outputLines;
	int32 currentOutputLine;

	real32 caretFlashCounter;
};

const int32 consoleLineLength = 255;

void initConsole(MemoryArena *debugArena, Console *console, int32 outputLineCount, BitmapFont *font, real32 height)
{
	console->isVisible = true;
	console->font = font;
	console->styles[CLS_Default].textColor   = color255(192, 192, 192, 255);
	console->styles[CLS_Input].textColor     = color255(255, 255, 255, 255);
	console->styles[CLS_InputEcho].textColor = color255(128, 128, 128, 255);
	console->styles[CLS_Error].textColor     = color255(255, 128, 128, 255);
	console->height = height;

	console->input = newStringBuffer(debugArena, consoleLineLength);
	console->outputLineCount = outputLineCount;
	console->outputLines = PushArray(debugArena, ConsoleOutputLine, console->outputLineCount);
	for (int32 i=0; i < console->outputLineCount; i++)
	{
		console->outputLines[i].buffer = newStringBuffer(debugArena, consoleLineLength);
	}
}

StringBuffer *consoleNextOutputLine(Console *console, ConsoleLineStyleID style=CLS_Default)
{
	ConsoleOutputLine *line = console->outputLines + console->currentOutputLine;
	line->style = style;

	StringBuffer *result = &line->buffer;
	console->currentOutputLine = WRAP(console->currentOutputLine + 1, console->outputLineCount);

	clear(result);

	return result;
}