#pragma once
#pragma warning(push)
#pragma warning(disable: 4100) // Disable unused-arg warnings for commands, as they all have to take the same args.

struct Command
{
	String name;
	void (*function)(Console*, TokenList*);
	int32 minArgs, maxArgs;

	Command(char *name, void (*function)(Console*, TokenList*), int32 minArgs, int32 maxArgs)
	{
		this->name = stringFromChars(name);
		this->function = function;
		this->minArgs = minArgs;
		this->maxArgs = maxArgs;
	}
};
#define ConsoleCommand(name) void cmd_##name(Console *console, TokenList *tokens)
Array<Command> consoleCommands(8);

ConsoleCommand(help)
{
	append(consoleNextOutputLine(console), "Available commands are:");
	for (int i=0; i < consoleCommands.count; i++)
	{
		Command cmd = consoleCommands[i];
		StringBuffer *line = consoleNextOutputLine(console);
		append(line, " - ");
		append(line, cmd.name);
	}
}

ConsoleCommand(resize_window)
{
	bool succeeded = false;

	String sWidth = tokens->tokens[1];
	String sHeight = tokens->tokens[2];
	
	int32 width = 0;
	int32 height = 0;
	if (asInt(sWidth, &width)   && (width > 0)
	 && asInt(sHeight, &height) && (height > 0))
	{
		StringBuffer *output = consoleNextOutputLine(console, CLS_Success);
		append(output, "Window resized to ");
		append(output, sWidth);
		append(output, " by ");
		append(output, sHeight);
		succeeded = true;
		resizeWindow(globalAppState.renderer, width, height);
	}

	if (!succeeded)
	{
		StringBuffer *output = consoleNextOutputLine(console, CLS_Error);
		append(output, "Usage: ");
		append(output, tokens->tokens[0]);
		append(output, " width height, where both width and height are positive integers");
	}
}

ConsoleCommand(reload_assets)
{
	reloadAssets(globalAppState.assets, &globalAppState.tempArena, globalAppState.renderer, &globalAppState.uiState);
}

#define CMD(name) #name, &cmd_##name
void initCommands(Console *console)
{
	append(&consoleCommands, Command(CMD(help), 0, 0));
	append(&consoleCommands, Command(CMD(resize_window), 2, 2));
	append(&consoleCommands, Command(CMD(reload_assets), 0, 0));

	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "Loaded %d commands. Type 'help' to list them.", consoleCommands.count);
	append(consoleNextOutputLine(console, CLS_Default), buffer);
}
#undef CMD

#pragma warning(pop)