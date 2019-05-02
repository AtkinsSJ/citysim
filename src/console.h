#pragma once

enum ConsoleLineStyleID
{
	CLS_Default,
	CLS_InputEcho,
	CLS_Success,
	CLS_Warning,
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

struct CommandShortcut
{
	KeyboardShortcut shortcut;
	String command;
};

struct Console;

struct Command
{
	String name;
	void (*function)(Console*, s32, String);
	s32 minArgs, maxArgs;

	Command(char *name, void (*function)(Console*, s32, String), s32 minArgs, s32 maxArgs)
	{
		this->name = makeString(name);
		this->function = function;
		this->minArgs = minArgs;
		this->maxArgs = maxArgs;
	}
};

struct Console
{
	struct BitmapFont *font;
	ConsoleLineStyle styles[CLS_COUNT];

	f32 currentHeight;
	f32 targetHeight;
	f32 openHeight; // % of screen height
	f32 maximisedHeight; // % of screen height
	f32 openSpeed; // % per second

	TextInput input;
	ChunkedArray<String> inputHistory;
	s32 inputHistoryCursor;
	s32 charWidth;

	s32 outputLineCount;
	ConsoleOutputLine *outputLines;
	s32 currentOutputLine;
	s32 scrollPos; // first line to draw, just above the console input

	ChunkedArray<Command> commands;
	ChunkedArray<CommandShortcut> commandShortcuts;
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
	consoleWriteLine(makeString(text), style);
}

inline s32 consoleMaxScrollPos(Console *console)
{
	s32 result = MIN(console->currentOutputLine-1, console->outputLineCount)-1;
	return result;
}

void initCommands(Console *console); // Implementation in commands.cpp

void loadConsoleKeyboardShortcuts(Console *console, Blob data, String filename);