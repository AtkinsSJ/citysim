#pragma once

// NB: ConsoleLineStyleID is in uitheme.h

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

	Command(String name, void (*function)(Console*, s32, String), s32 minArgs=0, s32 maxArgs=0)
	{
		this->name = name;
		this->function = function;
		this->minArgs = minArgs;
		this->maxArgs = maxArgs;
	}
};

struct Console
{
	AssetRef style;

	f32 currentHeight;
	f32 targetHeight;
	f32 openHeight; // % of screen height
	f32 maximisedHeight; // % of screen height
	f32 openSpeed; // % per second

	TextInput input;
	ChunkedArray<String> inputHistory;
	s32 inputHistoryCursor;

	ChunkedArray<ConsoleOutputLine> outputLines;
	ScrollbarState scrollbar;

	HashTable<Command> commands;
	ChunkedArray<CommandShortcut> commandShortcuts;
};

Console *globalConsole;
const s32 consoleLineLength = 255;

void initConsole(MemoryArena *debugArena, f32 openHeight, f32 maximisedHeight, f32 openSpeed);
void updateAndRenderConsole(Console *console);

void initCommands(Console *console); // Implementation in commands.cpp
void loadConsoleKeyboardShortcuts(Console *console, Blob data, String filename);
void consoleHandleCommand(Console *console, String commandInput);

void consoleWriteLine(String text, ConsoleLineStyleID style = CLS_Default);

// Private
Rect2I getConsoleScrollbarBounds(Console *console);

inline s32 consoleMaxScrollPos(Console *console)
{
	return truncate32(console->outputLines.count - 1);
}
