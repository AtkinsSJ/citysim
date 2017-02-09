#pragma once
#pragma warning(push)
#pragma warning(disable: 4100) // Disable unused-arg warnings for commands, as they all have to take the same args.

struct Command
{
	String name;
	void (*function)(Console*, TokenList*);

	Command(char *name, void (*function)(Console*, TokenList*))
	{
		this->name = stringFromChars(name);
		this->function = function;
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
		append(line, "    ");
		append(line, cmd.name);
	}
}

ConsoleCommand(resize_window)
{
	bool succeeded = false;

	if (tokens->count == 3)
	{
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
			resizeWindow(globalDebugState->renderer, width, height);
		}
	}

	if (!succeeded)
	{
		StringBuffer *output = consoleNextOutputLine(console);
		append(output, "Usage: ");
		append(output, tokens->tokens[0]);
		append(output, " width height");
	}
}

void initCommands(Console *console)
{
	append(&consoleCommands, Command("help", &cmd_help));
	append(&consoleCommands, Command("resize_window", &cmd_resize_window));

	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "Loaded %d commands. Type 'help' to list them.", consoleCommands.count);
	append(consoleNextOutputLine(console, CLS_Default), buffer);
}

#pragma warning(pop)