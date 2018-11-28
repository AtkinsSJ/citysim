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
	bool isVisible;
	struct BitmapFont *font;
	ConsoleLineStyle styles[CLS_COUNT];
	f32 height;

	TextInput input;
	s32 charWidth;

	s32 outputLineCount;
	ConsoleOutputLine *outputLines;
	s32 currentOutputLine;

	f32 caretFlashCounter;
};
Console *globalConsole;

const s32 consoleLineLength = 255;

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