#pragma once

enum ConsoleLineStyleID
{
	CLS_Default,
	CLS_InputEcho,
	CLS_Success,
	CLS_Warning,
	CLS_Error,

	CLS_COUNT
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

	Command(char *name, void (*function)(Console*, s32, String), s32 minArgs=0, s32 maxArgs=-1)
	{
		this->name = makeString(name);
		this->function = function;
		this->minArgs = minArgs;
		if (maxArgs == -1)
		{
			this->maxArgs = minArgs;
		}
		else
		{
			this->maxArgs = maxArgs;
		}
	}
};

struct Console
{
	f32 currentHeight;
	f32 targetHeight;
	f32 openHeight; // % of screen height
	f32 maximisedHeight; // % of screen height
	f32 openSpeed; // % per second

	TextInput input;
	ChunkedArray<String> inputHistory;
	s32 inputHistoryCursor;

	ChunkedArray<ConsoleOutputLine> outputLines;
	s32 scrollPos; // first line to draw, just above the console input

	ChunkedArray<Command> commands;
	ChunkedArray<CommandShortcut> commandShortcuts;
};

Console *globalConsole;
const s32 consoleLineLength = 255;

void initConsole(MemoryArena *debugArena, f32 openHeight, f32 maximisedHeight, f32 openSpeed);
void updateConsole(Console *console);
void renderConsole(Console *console);

void initCommands(Console *console); // Implementation in commands.cpp
void loadConsoleKeyboardShortcuts(Console *console, Blob data, String filename);
void consoleHandleCommand(Console *console, String commandInput);

void consoleWriteLine(String text, ConsoleLineStyleID style=CLS_Default);
inline void consoleWriteLine(char *text, ConsoleLineStyleID style=CLS_Default)
{
	consoleWriteLine(makeString(text), style);
}

inline s32 consoleMaxScrollPos(Console *console)
{
	return truncate32(console->outputLines.count - 1);
}
