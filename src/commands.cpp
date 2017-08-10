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
	consoleWriteLine("Available commands are:");
	for (int i=0; i < consoleCommands.count; i++)
	{
		consoleWriteLine(myprintf(" - {0}", {consoleCommands[i].name}));
	}
}

ConsoleCommand(window_size)
{
	// The only place we can access the window size is via the renderer's UI camera!
	// UGH this is so hacky. But I guess that's how the debug console should be?
	V2 screenSize = globalAppState.renderer->uiBuffer.camera.size;

	consoleWriteLine(myprintf("Window size is {0} by {1}", {formatInt((int32)screenSize.x), formatInt((int32)screenSize.y)}), CLS_Success);
}

ConsoleCommand(resize_window)
{
	bool succeeded = false;

	String sWidth = tokens->tokens[1];
	String sHeight = tokens->tokens[2];
	
	int64 width = 0;
	int64 height = 0;
	if (asInt(sWidth, &width)   && (width > 0)
	 && asInt(sHeight, &height) && (height > 0))
	{
		consoleWriteLine(myprintf("Window resizes to {0} by {1}", {sWidth, sHeight}), CLS_Success);

		succeeded = true;
		resizeWindow(globalAppState.renderer, (int32)width, (int32)height);
	}

	if (!succeeded)
	{
		consoleWriteLine(myprintf("Usage: {0} width height, where both width and height are positive integers",
								{tokens->tokens[0]}), CLS_Error);
	}
}

ConsoleCommand(reload_assets)
{
	reloadAssets(globalAppState.assets, &globalAppState.globalTempArena, globalAppState.renderer, &globalAppState.uiState);
}

#define CMD(name) #name, &cmd_##name
void initCommands(Console *console)
{
	append(&consoleCommands, Command(CMD(help), 0, 0));
	append(&consoleCommands, Command(CMD(window_size), 0, 0));
	append(&consoleCommands, Command(CMD(resize_window), 2, 2));
	append(&consoleCommands, Command(CMD(reload_assets), 0, 0));

	consoleWriteLine(myprintf("Loaded {0} commands. Type 'help' to list them.", {formatInt(consoleCommands.count)}), CLS_Default);
}
#undef CMD

#pragma warning(pop)