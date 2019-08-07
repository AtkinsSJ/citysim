#pragma once
#pragma warning(push)
#pragma warning(disable: 4100) // Disable unused-arg warnings for commands, as they all have to take the same args.

#define ConsoleCommand(name) void cmd_##name(Console *console, s32 argumentsCount, String arguments)

static bool checkInGame()
{
	bool inGame = (globalAppState.gameState != null);
	if (!inGame)
	{
		consoleWriteLine("You can only do that when a game is in progress!", CLS_Error);
	}
	return inGame;
}

ConsoleCommand(exit)
{
	consoleWriteLine("Quitting game...", CLS_Success);
	globalAppState.appStatus = AppStatus_Quit;
}

ConsoleCommand(funds)
{
	String remainder = arguments;
	if (!checkInGame()) return;

	String sAmount = nextToken(remainder, &remainder);
	s64 amount = 0;
	if (asInt(sAmount, &amount))
	{
		consoleWriteLine(myprintf("Set funds to {0}", {sAmount}), CLS_Success);
		globalAppState.gameState->city.funds = (s32) amount;
	}
	else
	{
		consoleWriteLine("Usage: funds amount, where amount is an integer", CLS_Error);
	}
}

ConsoleCommand(hello)
{
	consoleWriteLine("Hello human!");
	consoleWriteLine(myprintf("Testing formatInt bases: 10:{0}, 16:{1}, 36:{2}, 8:{3}, 2:{4}", {formatInt(123456, 10), formatInt(123456, 16), formatInt(123456, 36), formatInt(123456, 8), formatInt(123456, 2)}));
}

ConsoleCommand(help)
{
	consoleWriteLine("Available commands are:");

	for (auto it = iterate(&globalConsole->commands);
		!it.isDone;
		next(&it))
	{
		Command *command = get(it);
		consoleWriteLine(myprintf(" - {0}", {command->name}));
	}
}

ConsoleCommand(reload_assets)
{
	reloadAssets();
}

ConsoleCommand(reload_settings)
{
	loadSettings(&globalAppState.settings);
	applySettings(&globalAppState.settings);
}

ConsoleCommand(show_layer)
{
	String remainder = arguments;
	if (!checkInGame()) return;
	
	// For now this is a toggle, but it'd be nice if we could say "show_paths true" or "show_paths 1" maybe
	if (argumentsCount == 0)
	{
		// Hide layers
		globalAppState.gameState->dataLayerToDraw = DataLayer_None;
		consoleWriteLine("Hiding data layers", CLS_Success);
	}
	else if (argumentsCount == 1)
	{
		String layerName = nextToken(remainder, &remainder);
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
		consoleWriteLine("Usage: show_layer (paths|power), or with no argument to hide the data layer", CLS_Error);
	}
}

ConsoleCommand(window_size)
{
	bool succeeded = false;
	if (argumentsCount == 2)
	{
		String remainder = arguments;

		String sWidth  = nextToken(remainder, &remainder);
		String sHeight = nextToken(remainder, &remainder);
		
		s64 width = 0;
		s64 height = 0;
		if (asInt(sWidth, &width)   && (width > 0)
		 && asInt(sHeight, &height) && (height > 0))
		{
			consoleWriteLine(myprintf("Window resized to {0} by {1}", {sWidth, sHeight}), CLS_Success);

			succeeded = true;
			resizeWindow((s32)width, (s32)height, false);
		}
	}
	else if (argumentsCount == 0)
	{
		V2 screenSize = renderer->uiCamera.size;
		consoleWriteLine(myprintf("Window size is {0} by {1}", {formatInt((s32)screenSize.x), formatInt((s32)screenSize.y)}), CLS_Success);

		succeeded = true;
	}

	if (!succeeded)
	{
		consoleWriteLine("Usage: window_size [width height], where both width and height are positive integers. If no width or height are provided, the current window size is returned.", CLS_Error);
	}
}

ConsoleCommand(zoom)
{
	String remainder = arguments;
	bool succeeded = false;

	if (argumentsCount == 0)
	{
		// list the zoom
		f32 zoom = renderer->worldCamera.zoom;
		consoleWriteLine(myprintf("Current zoom is {0}", {formatFloat(zoom, 3)}), CLS_Success);
		succeeded = true;
	}
	else if (argumentsCount == 1)
	{
		// set the zoom
		// TODO: We don't have a float-parsing function yet, so we're stuck with ints!
		s64 requestedZoom;
		if (asInt(nextToken(remainder, &remainder), &requestedZoom))
		{
			f32 newZoom = (f32) requestedZoom;
			renderer->worldCamera.zoom = newZoom;
			consoleWriteLine(myprintf("Set zoom to {0}", {formatFloat(newZoom, 3)}), CLS_Success);
			succeeded = true;
		}
	}

	if (!succeeded)
	{
		consoleWriteLine("Usage: zoom (scale), where scale is an integer, or with no argument to list the current zoom", CLS_Error);
	}
}

#define CMD(name) #name, &cmd_##name
void initCommands(Console *console)
{
	// NB: a max-arguments value of -1 means "no maximum"
	append(&console->commands, Command(CMD(help), 0, 0));
	append(&console->commands, Command(CMD(exit), 0, 0));
	append(&console->commands, Command(CMD(funds), 1, 1));
	append(&console->commands, Command(CMD(hello), 0, 1));
	append(&console->commands, Command(CMD(reload_assets), 0, 0));
	append(&console->commands, Command(CMD(reload_settings), 0, 0));
	append(&console->commands, Command(CMD(show_layer), 0, 1));
	append(&console->commands, Command(CMD(window_size), 0, 2));
	append(&console->commands, Command(CMD(zoom), 0, 1));
}
#undef CMD

#pragma warning(pop)