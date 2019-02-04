#pragma once
#pragma warning(push)
#pragma warning(disable: 4100) // Disable unused-arg warnings for commands, as they all have to take the same args.

struct Command
{
	String name;
	void (*function)(Console*, TokenList*);
	s32 minArgs, maxArgs;

	Command(char *name, void (*function)(Console*, TokenList*), s32 minArgs, s32 maxArgs)
	{
		this->name = stringFromChars(name);
		this->function = function;
		this->minArgs = minArgs;
		this->maxArgs = maxArgs;
	}
};
#define ConsoleCommand(name) void cmd_##name(Console *console, TokenList *tokens)
Array<Command> consoleCommands;

ConsoleCommand(help)
{
	consoleWriteLine("Available commands are:");
	for (u32 i=0; i < consoleCommands.count; i++)
	{
		consoleWriteLine(myprintf(" - {0}", {consoleCommands[i].name}));
	}
}

ConsoleCommand(hello)
{
	consoleWriteLine("Hello human!");
	consoleWriteLine(myprintf("Testing formatInt bases: 10:{0}, 16:{1}, 36:{2}, 8:{3}, 2:{4}", {formatInt(123456, 10), formatInt(123456, 16), formatInt(123456, 36), formatInt(123456, 8), formatInt(123456, 2)}));
}

ConsoleCommand(window_size)
{
	// The only place we can access the window size is via the renderer's UI camera!
	// UGH this is so hacky. But I guess that's how the debug console should be?
	V2 screenSize = globalAppState.renderer->uiBuffer.camera.size;

	consoleWriteLine(myprintf("Window size is {0} by {1}", {formatInt((s32)screenSize.x), formatInt((s32)screenSize.y)}), CLS_Success);
}

ConsoleCommand(resize_window)
{
	bool succeeded = false;

	String sWidth = tokens->tokens[1];
	String sHeight = tokens->tokens[2];
	
	s64 width = 0;
	s64 height = 0;
	if (asInt(sWidth, &width)   && (width > 0)
	 && asInt(sHeight, &height) && (height > 0))
	{
		consoleWriteLine(myprintf("Window resized to {0} by {1}", {sWidth, sHeight}), CLS_Success);

		succeeded = true;
		resizeWindow(globalAppState.renderer, (s32)width, (s32)height);
	}

	if (!succeeded)
	{
		consoleWriteLine(myprintf("Usage: {0} width height, where both width and height are positive integers",
								{tokens->tokens[0]}), CLS_Error);
	}
}

ConsoleCommand(reload_assets)
{
	reloadAssets(globalAppState.assets, globalAppState.renderer, &globalAppState.uiState);
}

ConsoleCommand(exit)
{
	consoleWriteLine("Quitting game...", CLS_Success);
	globalAppState.appStatus = AppStatus_Quit;
}

ConsoleCommand(funds)
{
	String sAmount = tokens->tokens[1];
	s64 amount = 0;
	if (asInt(sAmount, &amount))
	{
		if (globalAppState.gameState != nullptr)
		{
			consoleWriteLine(myprintf("Set funds to {0}", {sAmount}), CLS_Success);
			globalAppState.gameState->city.funds = (s32) amount;
		}
		else
		{
			consoleWriteLine("You can only do that when a game is in progress!", CLS_Error);
		}
	}
	else
	{
		consoleWriteLine(myprintf("Usage: {0} amount, where amount is an integer",
								{tokens->tokens[0]}), CLS_Error);
	}
}

ConsoleCommand(show_layer)
{
	// For now this is a toggle, but it'd be nice if we could say "show_paths true" or "show_paths 1" maybe
	if (globalAppState.gameState != nullptr)
	{
		if (tokens->count == 1)
		{
			// Hide layers
			globalAppState.gameState->dataLayerToDraw = DataLayer_None;
			consoleWriteLine("Hiding data layers", CLS_Success);
		}
		else if (tokens->count == 2)
		{
			String layerName = tokens->tokens[1];
			if (equals(layerName, "paths"))
			{
				globalAppState.gameState->dataLayerToDraw = DataLayer_Paths;
				consoleWriteLine("Showing paths layer", CLS_Success);
			}
			else if (equals(layerName, "power"))
			{
				globalAppState.gameState->dataLayerToDraw = DataLayer_Power;
				consoleWriteLine("Showing power layer", CLS_Success);
			}
		}
		else
		{
			consoleWriteLine(myprintf("Usage: {0} (paths|power), or with no argument to hide the data layer",
								{tokens->tokens[0]}), CLS_Error);
		}
	}
	else
	{
		consoleWriteLine("You can only do that when a game is in progress!", CLS_Error);
	}

}

s32 compareCommands(Command *a, Command *b)
{
	return compare(a->name, b->name);
}

#define CMD(name) #name, &cmd_##name
void initCommands(Console *console)
{
	initialiseArray(&consoleCommands, 64);

	append(&consoleCommands, Command(CMD(help), 0, 0));
	append(&consoleCommands, Command(CMD(hello), 0, 1));
	append(&consoleCommands, Command(CMD(window_size), 0, 0));
	append(&consoleCommands, Command(CMD(resize_window), 2, 2));
	append(&consoleCommands, Command(CMD(reload_assets), 0, 0));
	append(&consoleCommands, Command(CMD(exit), 0, 0));
	append(&consoleCommands, Command(CMD(funds), 1, 1));
	append(&consoleCommands, Command(CMD(show_layer), 0, 1));

	sortInPlace(&consoleCommands, compareCommands);

	consoleWriteLine(myprintf("Loaded {0} commands. Type 'help' to list them.", {formatInt(consoleCommands.count)}), CLS_Default);
}
#undef CMD

#pragma warning(pop)