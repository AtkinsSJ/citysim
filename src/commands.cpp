#pragma once
#pragma warning(push)
#pragma warning(disable: 4100) // Disable unused-arg warnings for commands, as they all have to take the same args.

#define ConsoleCommand(name) void cmd_##name(Console *console, s32 argumentsCount, String arguments)

static bool checkInGame()
{
	bool inGame = (globalAppState.gameState != null);
	if (!inGame)
	{
		consoleWriteLine("You can only do that when a game is in progress!"_s, CLS_Error);
	}
	return inGame;
}

ConsoleCommand(debug_tools)
{
	if (!checkInGame()) return;

	UIState *uiState = globalAppState.uiState;
	GameState *gameState = globalAppState.gameState;
	V2I windowPos = v2i(renderer->uiCamera.pos + renderer->uiCamera.size);

	showWindow(uiState, "Debug Tools"_s, 250, 200, windowPos, "default"_s, WinFlag_AutomaticHeight | WinFlag_Unique, debugToolsWindowProc, gameState);
}

ConsoleCommand(exit)
{
	consoleWriteLine("Quitting game..."_s, CLS_Success);
	globalAppState.appStatus = AppStatus_Quit;
}

ConsoleCommand(funds)
{
	String remainder = arguments;
	if (!checkInGame()) return;

	String sAmount = nextToken(remainder, &remainder);
	Maybe<s64> amount = asInt(sAmount);
	if (amount.isValid)
	{
		consoleWriteLine(myprintf("Set funds to {0}"_s, {sAmount}), CLS_Success);
		globalAppState.gameState->city.funds = truncate32(amount.value);
	}
	else
	{
		consoleWriteLine("Usage: funds amount, where amount is an integer"_s, CLS_Error);
	}
}

ConsoleCommand(generate)
{
	if (!checkInGame()) return;

	City *city = &globalAppState.gameState->city;
	// TODO: Some kind of reset would be better than this, but this is temporary until we add
	// proper terrain generation and UI, so meh.
	if (city->buildings.count > 0)
	{
		demolishRect(city, city->bounds);
		city->highestBuildingID = 0;
	}
	generateTerrain(city, &globalAppState.gameState->gameRandom);

	consoleWriteLine("Generated new map"_s, CLS_Success);
}

ConsoleCommand(hello)
{
	consoleWriteLine("Hello human!"_s);
	consoleWriteLine(myprintf("Testing formatInt bases: 10:{0}, 16:{1}, 36:{2}, 8:{3}, 2:{4}"_s, {formatInt(123456, 10), formatInt(123456, 16), formatInt(123456, 36), formatInt(123456, 8), formatInt(123456, 2)}));
}

ConsoleCommand(help)
{
	consoleWriteLine("Available commands are:"_s);

	for (auto it = iterate(&globalConsole->commands);
		!it.isDone;
		next(&it))
	{
		Command *command = get(it);
		consoleWriteLine(myprintf(" - {0}"_s, {command->name}));
	}
}

ConsoleCommand(mark_all_dirty)
{
	City *city = &globalAppState.gameState->city;
	markAreaDirty(city, city->bounds);
}

ConsoleCommand(reload_assets)
{
	reloadAssets();
}

ConsoleCommand(reload_settings)
{
	loadSettings();
	applySettings();
}

ConsoleCommand(show_layer)
{
	String remainder = arguments;
	if (!checkInGame()) return;
	
	if (argumentsCount == 0)
	{
		// Hide layers
		globalAppState.gameState->dataLayerToDraw = DataLayer_None;
		consoleWriteLine("Hiding data layers"_s, CLS_Success);
	}
	else if (argumentsCount == 1)
	{
		String layerName = nextToken(remainder, &remainder);
		if (equals(layerName, "crime"_s))
		{
			globalAppState.gameState->dataLayerToDraw = DataLayer_Crime;
			consoleWriteLine("Showing crime layer"_s, CLS_Success);
		}
		else if (equals(layerName, "des_res"_s))
		{
			globalAppState.gameState->dataLayerToDraw = DataLayer_Desirability_Residential;
			consoleWriteLine("Showing residential desirability"_s, CLS_Success);
		}
		else if (equals(layerName, "des_com"_s))
		{
			globalAppState.gameState->dataLayerToDraw = DataLayer_Desirability_Commercial;
			consoleWriteLine("Showing commercial desirability"_s, CLS_Success);
		}
		else if (equals(layerName, "des_ind"_s))
		{
			globalAppState.gameState->dataLayerToDraw = DataLayer_Desirability_Industrial;
			consoleWriteLine("Showing industrial desirability"_s, CLS_Success);
		}
		else if (equals(layerName, "fire"_s))
		{
			globalAppState.gameState->dataLayerToDraw = DataLayer_Fire;
			consoleWriteLine("Showing fire layer"_s, CLS_Success);
		}
		else if (equals(layerName, "health"_s))
		{
			globalAppState.gameState->dataLayerToDraw = DataLayer_Health;
			consoleWriteLine("Showing health layer"_s, CLS_Success);
		}
		else if (equals(layerName, "land_value"_s))
		{
			globalAppState.gameState->dataLayerToDraw = DataLayer_LandValue;
			consoleWriteLine("Showing land value layer"_s, CLS_Success);
		}
		else if (equals(layerName, "pollution"_s))
		{
			globalAppState.gameState->dataLayerToDraw = DataLayer_Pollution;
			consoleWriteLine("Showing pollution layer"_s, CLS_Success);
		}
		else if (equals(layerName, "power"_s))
		{
			globalAppState.gameState->dataLayerToDraw = DataLayer_Power;
			consoleWriteLine("Showing power layer"_s, CLS_Success);
		}
		else
		{
			consoleWriteLine("Usage: show_layer (layer_name), or with no argument to hide the data layer. Layer names are: crime, des_res, des_com, des_ind, fire, health, land_value, pollution, power"_s, CLS_Error);
		}
	}
}

ConsoleCommand(window_size)
{
	if (argumentsCount == 2)
	{
		String remainder = arguments;

		String sWidth  = nextToken(remainder, &remainder);
		String sHeight = nextToken(remainder, &remainder);
		
		Maybe<s64> width = asInt(sWidth);
		Maybe<s64> height = asInt(sHeight);
		if (width.isValid && (width.value > 0)
		 && height.isValid && (height.value > 0))
		{
			consoleWriteLine(myprintf("Window resized to {0} by {1}"_s, {sWidth, sHeight}), CLS_Success);

			resizeWindow(truncate32(width.value), truncate32(height.value), false);
		}
		else
		{
			consoleWriteLine("Usage: window_size [width height], where both width and height are positive integers. If no width or height are provided, the current window size is returned."_s, CLS_Error);
		}
	}
	else if (argumentsCount == 0)
	{
		V2 screenSize = renderer->uiCamera.size;
		consoleWriteLine(myprintf("Window size is {0} by {1}"_s, {formatInt((s32)screenSize.x), formatInt((s32)screenSize.y)}), CLS_Success);
	}
	else
	{
		consoleWriteLine("Usage: window_size [width height], where both width and height are positive integers. If no width or height are provided, the current window size is returned."_s, CLS_Error);
	}
}

ConsoleCommand(zoom)
{
	String remainder = arguments;

	if (argumentsCount == 0)
	{
		// list the zoom
		f32 zoom = renderer->worldCamera.zoom;
		consoleWriteLine(myprintf("Current zoom is {0}"_s, {formatFloat(zoom, 3)}), CLS_Success);
	}
	else if (argumentsCount == 1)
	{
		// set the zoom
		Maybe<f64> requestedZoom = asFloat(nextToken(remainder, &remainder));
		if (requestedZoom.isValid)
		{
			f32 newZoom = (f32) requestedZoom.value;
			renderer->worldCamera.zoom = newZoom;
			consoleWriteLine(myprintf("Set zoom to {0}"_s, {formatFloat(newZoom, 3)}), CLS_Success);
		}
		else
		{
			consoleWriteLine("Usage: zoom (scale), where scale is a float, or with no argument to list the current zoom"_s, CLS_Error);
		}
	}
}

#define CMD(name) #name##_s, &cmd_##name
void initCommands(Console *console)
{
	// NB: a max-arguments value of -1 means "no maximum"
	append(&console->commands, Command(CMD(help)));
	append(&console->commands, Command(CMD(debug_tools)));
	append(&console->commands, Command(CMD(exit)));
	append(&console->commands, Command(CMD(funds), 1));
	append(&console->commands, Command(CMD(generate)));
	append(&console->commands, Command(CMD(hello), 0, 1));
	append(&console->commands, Command(CMD(mark_all_dirty)));
	append(&console->commands, Command(CMD(reload_assets)));
	append(&console->commands, Command(CMD(reload_settings)));
	append(&console->commands, Command(CMD(show_layer), 0, 1));
	append(&console->commands, Command(CMD(window_size), 0, 2));
	append(&console->commands, Command(CMD(zoom), 0, 1));
}
#undef CMD

#pragma warning(pop)